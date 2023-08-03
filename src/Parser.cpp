#include "Parser.hpp"

#include <iostream>

#include "Config.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Parser/Parse.hpp"
#include "Parser/ParseHelper.hpp"
#include "Passes/Cleanup.hpp"
#include "Passes/Simplify.hpp"
#include "Passes/TypeAssign.hpp"
#include "Utils.hpp"

namespace sc
{
///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Module ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Module::Module(Context &ctx, StringRef id, StringRef path, StringRef code, bool is_main_module)
	: ctx(ctx), id(id), path(path), code(code), tokens(), ptree(nullptr),
	  is_main_module(is_main_module)
{}
Module::~Module() {}
bool Module::tokenize()
{
	lex::Tokenizer tokenizer(ctx, this);
	return tokenizer.tokenize(code, tokens);
}
bool Module::parseTokens()
{
	ParseHelper p(ctx, this, tokens);
	Parsing parsing(ctx);
	return parsing.parseBlock(p, (StmtBlock *&)ptree, false);
}
bool Module::executePasses(PassManager &pm) { return pm.visit(ptree); }
StringRef Module::getID() const { return id; }
StringRef Module::getPath() const { return path; }
StringRef Module::getCode() const { return code; }
const Vector<lex::Lexeme> &Module::getTokens() const { return tokens; }
Stmt *&Module::getParseTree() { return ptree; }
bool Module::isMainModule() const { return is_main_module; }
void Module::dumpTokens() const
{
	std::cout << "Source: " << path << "\n";
	for(auto &t : tokens) {
		std::cout << t.str() << "\n";
	}
}
void Module::dumpParseTree() const
{
	std::cout << "Source: " << path << "\n";
	ptree->disp(false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// RAIIParser /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

RAIIParser::RAIIParser(args::ArgParser &args)
	: args(args), ctx(this), defaultpmpermodule(ctx), defaultpmcombined(ctx),
	  mainmodule(nullptr)
{
	defaultpmpermodule.add<TypeAssignPass>();
	defaultpmcombined.add<SimplifyPass>();
	defaultpmcombined.add<CleanupPass>();
}
RAIIParser::~RAIIParser()
{
	for(auto &m : modules) delete m.second;
}

bool RAIIParser::init()
{
	// import preludes
	Vector<String> preludes = {"core"};
	for(auto &prelude : preludes) {
		if(!IsValidSource(prelude)) {
			std::cerr << "Prelude source '" << prelude
				  << "' not found, cannot continue!\n";
			return false;
		}
		if(!parse(prelude, false)) return false;
	}
	return true;
}

void RAIIParser::combineAllModules()
{
	if(modulestack.size() <= 1) return;
	Vector<Stmt *> allmodstmts;
	for(auto &mpath : moduleorder) {
		Module *mod		 = modules[mpath];
		StmtBlock *modptree	 = as<StmtBlock>(mod->getParseTree());
		Vector<Stmt *> &modstmts = modptree->getStmts();
		allmodstmts.insert(allmodstmts.end(), modstmts.begin(), modstmts.end());
		modstmts.clear();
	}

	StmtBlock *mainptree	  = as<StmtBlock>(mainmodule->getParseTree());
	Vector<Stmt *> &mainstmts = mainptree->getStmts();
	mainstmts.insert(mainstmts.begin(), allmodstmts.begin(), allmodstmts.end());

	ssize_t count = modulestack.size() - 1;
	while(count--) {
		modulestack.pop_back();
	}
}
Module *RAIIParser::addModule(StringRef path, bool main_module, StringRef code)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;

	if(code.empty()) {
		String _code;
		if(!fs::read(String(path), _code)) {
			return nullptr;
		}
		code = ctx.moveStr(std::move(_code));
	}
	StringRef id = ctx.strFrom(modulestack.size());

	Module *mod = new Module(ctx, id, path, code, main_module);
	Pointer<Module> mptr(mod);

	modulestack.push_back(path);

	if(!mod->tokenize() || !mod->parseTokens() /* || !mod->assignType(types)*/) {
		modulestack.pop_back();
		return nullptr;
	}

	mptr.unset();
	modules[path] = mod;
	return mod;
}
bool RAIIParser::parse(const String &_path, bool main_module, StringRef code)
{
	if(hasModule(_path)) {
		std::cerr << "cannot parse an existing source: " << _path << "\n";
		return false;
	}

	String wd = fs::getCWD();
	fs::setCWD(fs::parentDir(_path));
	size_t src_id  = 0;
	StringRef path = ctx.strFrom(_path);
	if(!addModule(path, main_module, code)) return false;
	bool res = modules[path]->executePasses(defaultpmpermodule);
	fs::setCWD(wd);
	if(!res) goto end;
	if(main_module) {
		mainmodule = modules[path];
		combineAllModules();
		res = modules[path]->executePasses(defaultpmcombined);
	} else {
		moduleorder.push_back(path);
	}
end:
	return res;
}

Module *RAIIParser::getModule(StringRef path)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;
	return nullptr;
}

void RAIIParser::dumpTokens(bool force)
{
	if(!args.has("tokens") && !force) return;

	printf("-------------------------------------------------- Token(s) "
	       "--------------------------------------------------\n");
	for(auto file = modulestack.rbegin(); file != modulestack.rend(); ++file) {
		printf("\n\n");
		modules[*file]->dumpTokens();
	}
}
void RAIIParser::dumpParseTree(bool force)
{
	if(!args.has("parse") && !args.has("semantic") && !force) return;

	printf("-------------------------------------------------- Parse Tree(s) "
	       "--------------------------------------------------\n");
	for(auto file = modulestack.rbegin(); file != modulestack.rend(); ++file) {
		printf("\n\n");
		modules[*file]->dumpParseTree();
	}
}

bool RAIIParser::IsValidSource(String &modname)
{
	static String import_dir = INSTALL_DIR "/include/scribe";
	if(modname.front() != '~' && modname.front() != '/' && modname.front() != '.') {
		if(fs::exists(import_dir + "/" + modname + ".sc")) {
			modname = fs::absPath(import_dir + "/" + modname + ".sc");
			return true;
		}
	} else {
		if(modname.front() == '~') {
			modname.erase(modname.begin());
			String home = fs::home();
			modname.insert(modname.begin(), home.begin(), home.end());
		}
		if(fs::exists(modname + ".sc")) {
			modname = fs::absPath(modname + ".sc");
			return true;
		}
	}
	return false;
}
} // namespace sc
