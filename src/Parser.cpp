/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Parser.hpp"

#include <unistd.h>

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

Module::Module(ErrMgr &err, Context &ctx, const std::string &id, const std::string &path,
	       const std::string &code, const bool &is_main_module)
	: err(err), ctx(ctx), id(id), path(path), code(code), tokens(), ptree(nullptr),
	  is_main_module(is_main_module)
{}
Module::~Module() {}
bool Module::tokenize()
{
	lex::Tokenizer tokenizer(this, err);
	return tokenizer.tokenize(code, tokens);
}
bool Module::parseTokens()
{
	ParseHelper p(this, tokens);
	Parsing parsing(err, ctx);
	return parsing.parse_block(p, (StmtBlock *&)ptree, false);
}
bool Module::executePasses(PassManager &pm)
{
	return pm.visit(ptree);
}
const std::string &Module::getID() const
{
	return id;
}
const std::string &Module::getPath() const
{
	return path;
}
const std::string &Module::getCode() const
{
	return code;
}
const std::vector<lex::Lexeme> &Module::getTokens() const
{
	return tokens;
}
Stmt *&Module::getParseTree()
{
	return ptree;
}
bool Module::isMainModule() const
{
	return is_main_module;
}
void Module::dumpTokens() const
{
	printf("Source: %s\n", path.c_str());
	for(auto &t : tokens) {
		printf("%s\n", t.str().c_str());
	}
}
void Module::dumpParseTree() const
{
	printf("Source: %s\n", path.c_str());
	ptree->disp(false);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// RAIIParser /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

RAIIParser::RAIIParser(args::ArgParser &args)
	: args(args), ctx(this), defaultpmpermodule(err, ctx), defaultpmcombined(err, ctx)
{
	defaultpmpermodule.add<TypeAssignPass>();
	defaultpmcombined.add<SimplifyPass>();
	defaultpmcombined.add<CleanupPass>();
}
RAIIParser::~RAIIParser()
{
	for(auto &m : modules) delete m.second;
}

void RAIIParser::combineAllModules()
{
	if(modulestack.size() <= 1) return;
	std::vector<Stmt *> allmodstmts;
	for(auto &mpath : moduleorder) {
		Module *mod		      = modules[mpath];
		StmtBlock *modptree	      = as<StmtBlock>(mod->getParseTree());
		std::vector<Stmt *> &modstmts = modptree->getStmts();
		allmodstmts.insert(allmodstmts.end(), modstmts.begin(), modstmts.end());
		modstmts.clear();
	}

	Module *mainmod		       = modules[modulestack.front()];
	StmtBlock *mainptree	       = as<StmtBlock>(mainmod->getParseTree());
	std::vector<Stmt *> &mainstmts = mainptree->getStmts();
	mainstmts.insert(mainstmts.begin(), allmodstmts.begin(), allmodstmts.end());

	size_t count = modulestack.size() - 1;
	while(count--) {
		modulestack.pop_back();
	}
}
Module *RAIIParser::addModule(const std::string &path, const bool &main_module)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;

	std::string code;
	if(!fs::read(path, code)) {
		return nullptr;
	}

	Module *mod =
	new Module(err, ctx, std::to_string(modulestack.size()), path, code, main_module);
	Pointer<Module> mptr(mod);

	modulestack.push_back(path);

	if(!mod->tokenize() || !mod->parseTokens() /* || !mod->assignType(types)*/) {
		err.show(stderr);
		modulestack.pop_back();
		return nullptr;
	}

	mptr.unset();
	modules[path] = mod;
	return mod;
}
bool RAIIParser::parse(const std::string &path, const bool &main_module)
{
	if(hasModule(path)) {
		fprintf(stderr, "cannot parse an existing source: %s\n", path.c_str());
		return false;
	}

	std::string wd = fs::getCWD();
	fs::setCWD(fs::parentDir(path));
	size_t src_id = 0;
	if(!addModule(path, main_module)) return false;
	bool res = modules[path]->executePasses(defaultpmpermodule);
	fs::setCWD(wd);
	if(!res) goto end;
	if(main_module) {
		combineAllModules();
		res = modules[path]->executePasses(defaultpmcombined);
	} else {
		moduleorder.push_back(path);
	}
end:
	if(!res) err.show(stderr);
	return res;
}
bool RAIIParser::hasModule(const std::string &path)
{
	return modules.find(path) != modules.end();
}
Module *RAIIParser::getModule(const std::string &path)
{
	auto res = modules.find(path);
	if(res != modules.end()) return res->second;
	return nullptr;
}
const std::vector<std::string> &RAIIParser::getModuleStack()
{
	return modulestack;
}
args::ArgParser &RAIIParser::getCommandArgs()
{
	return args;
}
ErrMgr &RAIIParser::getErrMgr()
{
	return err;
}
Context &RAIIParser::getContext()
{
	return ctx;
}
void RAIIParser::dumpTokens(const bool &force)
{
	if(!args.has("tokens") && !force) return;

	printf("-------------------------------------------------- Token(s) "
	       "--------------------------------------------------\n");
	for(auto file = modulestack.rbegin(); file != modulestack.rend(); ++file) {
		printf("\n\n");
		modules[*file]->dumpTokens();
	}
}
void RAIIParser::dumpParseTree(const bool &force)
{
	if(!args.has("parse") && !args.has("semantic") && !force) return;

	printf("-------------------------------------------------- Parse Tree(s) "
	       "--------------------------------------------------\n");
	for(auto file = modulestack.rbegin(); file != modulestack.rend(); ++file) {
		printf("\n\n");
		modules[*file]->dumpParseTree();
	}
}
} // namespace sc
