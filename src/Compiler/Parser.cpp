#include "Parser.hpp"

#include "AST/Parse.hpp"
#include "AST/ParseHelper.hpp"
#include "Core.hpp"
#include "FS.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace sc
{
///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// Module ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Module::Module(Context &ctx, String &&id, const String &path, StringRef code, bool is_main_module)
	: ctx(ctx), id(id), path(path), code(code), tokens(), ptree(nullptr),
	  is_main_module(is_main_module)
{}
Module::Module(Context &ctx, String &&id, const String &path, String &&code, bool is_main_module)
	: ctx(ctx), id(id), path(path), code(std::move(code)), tokens(), ptree(nullptr),
	  is_main_module(is_main_module)
{}
Module::~Module()
{
	// for(auto &i : instrs) delete i;
}
bool Module::tokenize()
{
	lex::Tokenizer tokenizer(ctx, this);
	return tokenizer.tokenize(code, tokens);
}
bool Module::parseTokens()
{
	AST::ParseHelper p(ctx, this, tokens);
	AST::Parsing parsing(ctx);
	return parsing.parseBlock(p, (AST::StmtBlock *&)ptree, false);
}
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
	// defaultpmpermodule.add<AST::ConstFoldPass>();
	// defaultpmpermodule.add<TypeAssignPass>();
	// defaultpmcombined.add<SimplifyPass>();
	// defaultpmcombined.add<CleanupPass>();
}
RAIIParser::~RAIIParser()
{
	for(auto &m : modules) delete m.second;
}

bool RAIIParser::init()
{
	// import preludes
	static Array<String, 2> preludes = {"prelude/core", "prelude/stringref"};
	// for(auto &prelude : preludes) {
	// 	if(!isValidSource(prelude)) {
	// 		std::cerr << "Prelude source '" << prelude
	// 			  << "' not found, cannot continue!\n";
	// 		return false;
	// 	}
	// 	if(!parse(prelude, false)) return false;
	// }
	return true;
}

void RAIIParser::combineAllModules()
{
	if(modulestack.size() <= 1) return;
	Vector<AST::Stmt *> allmodstmts;
	for(auto &mpath : moduleorder) {
		Module *mod		      = modules[mpath];
		AST::StmtBlock *modptree      = as<AST::StmtBlock>(mod->getParseTree());
		Vector<AST::Stmt *> &modstmts = modptree->getStmts();
		allmodstmts.insert(allmodstmts.end(), modstmts.begin(), modstmts.end());
		modstmts.clear();
	}

	AST::StmtBlock *mainptree      = as<AST::StmtBlock>(mainmodule->getParseTree());
	Vector<AST::Stmt *> &mainstmts = mainptree->getStmts();
	mainstmts.insert(mainstmts.begin(), allmodstmts.begin(), allmodstmts.end());

	ssize_t count = modulestack.size() - 1;
	while(count-- > 0) {
		modulestack.pop_back();
	}
}
Module *RAIIParser::addModule(const String &path, bool main_module, StringRef code)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;

	String _code;
	if(code.empty() && !fs::read(path.c_str(), _code)) return nullptr;
	Module *mod = nullptr;
	if(_code.empty()) {
		mod = new Module(ctx, toString(modulestack.size()), path, code, main_module);
	} else {
		mod =
		new Module(ctx, toString(modulestack.size()), path, std::move(_code), main_module);
	}

	modulestack.push_back(mod->getPath());
	if(!mod->tokenize() || !mod->parseTokens()) {
		modulestack.pop_back();
		delete mod;
		return nullptr;
	}

	modules[mod->getPath()] = mod;
	return mod;
}
bool RAIIParser::parse(const String &path, bool main_module, StringRef code)
{
	if(hasModule(path)) {
		logger.fatal("cannot parse an existing source: ", path);
		return false;
	}

	String wd	 = fs::getCWD();
	String parentdir = String(fs::parentDir(path));
	if(!parentdir.empty()) fs::setCWD(parentdir.c_str());
	size_t src_id = 0;
	if(!addModule(path, main_module, code)) return false;
	Module *mod = modules[path];
	bool res    = mod->executePasses(defaultpmpermodule);
	fs::setCWD(wd.c_str());
	if(!res) goto end;
	if(main_module) {
		mainmodule = mod;
		combineAllModules();
		res = mod->executePasses(defaultpmcombined);
	} else {
		moduleorder.push_back(mod->getPath());
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

void RAIIParser::dumpTokens()
{
	std::cout << "-------------------------------------------------- Token(s) "
		     "--------------------------------------------------\n";
	mainmodule->dumpTokens();
}
void RAIIParser::dumpParseTree()
{
	std::cout << "-------------------------------------------------- Parse Tree(s) "
		     "--------------------------------------------------\n";
	mainmodule->dumpParseTree();
}

bool RAIIParser::isValidSource(String &modname)
{
	static String import_dir = fs::pathFrom(INSTALL_DIR, "lib", "scribe");
	if(modname.front() != '~' && modname.front() != '/' && modname.front() != '\\' &&
	   modname.front() != '.')
	{
		String tmp = fs::pathFrom(import_dir, modname + ".sc");
		if(fs::exists(tmp)) {
			modname = fs::absPath(tmp.c_str());
			return true;
		}
	} else {
		if(modname.front() == '~') {
			modname.erase(modname.begin());
			StringRef home = fs::home();
			modname.insert(modname.begin(), home.begin(), home.end());
		}
		if(fs::exists(modname + ".sc")) {
			modname = fs::absPath((modname + ".sc").c_str());
			return true;
		}
	}
	return false;
}
} // namespace sc
