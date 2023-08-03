#pragma once

#include "Args.hpp"
#include "Lex.hpp"
#include "Parser/Stmts.hpp"
#include "Passes/Base.hpp"

namespace sc
{

class Module
{
	Context &ctx;

	StringRef id;
	StringRef path;
	StringRef code;
	Vector<lex::Lexeme> tokens;
	Stmt *ptree;
	bool is_main_module;

public:
	Module(Context &ctx, StringRef id, StringRef path, StringRef code, bool is_main_module);
	~Module();

	bool tokenize();
	bool parseTokens();
	bool executePasses(PassManager &pm);

	StringRef getID() const;
	StringRef getPath() const;
	StringRef getCode() const;
	const Vector<lex::Lexeme> &getTokens() const;
	Stmt *&getParseTree();
	bool isMainModule() const;

	void dumpTokens() const;
	void dumpParseTree() const;
};

class RAIIParser
{
	args::ArgParser &args;

	Context ctx;

	// default pms that run:
	// 1. on each module
	// 2. once all modules are combined
	PassManager defaultpmpermodule, defaultpmcombined;

	// as new sources are imported, they'll be pushed back
	Vector<StringRef> modulestack;

	// the iteration of this list will give the order of imports
	// this list does NOT contain the main module
	Vector<StringRef> moduleorder;

	Map<StringRef, Module *> modules;

	Module *mainmodule;

	Module *addModule(StringRef path, bool main_module, StringRef code);

public:
	RAIIParser(args::ArgParser &args);
	~RAIIParser();

	bool init();

	// if code is not empty, file won't be read/checked
	bool parse(const String &_path, bool main_module = false, StringRef code = "");
	void combineAllModules();

	inline bool hasModule(StringRef path) { return modules.find(path) != modules.end(); }
	inline const Vector<StringRef> &getModuleStack() { return modulestack; }
	inline args::ArgParser &getCommandArgs() { return args; }
	inline Context &getContext() { return ctx; }
	inline Module *getMainModule() { return mainmodule; }
	Module *getModule(StringRef path);

	// force ignores arg parser
	void dumpTokens(bool force);
	void dumpParseTree(bool force);

	// search for a source (import) in the import/include paths
	// updates the modname parameter if the file is found
	bool IsValidSource(String &modname);
};
} // namespace sc
