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

#ifndef PARSER_HPP
#define PARSER_HPP

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

	Module *addModule(StringRef path, bool main_module);

public:
	RAIIParser(args::ArgParser &args);
	~RAIIParser();

	bool parse(const String &_path, bool main_module = false);
	void combineAllModules();

	bool hasModule(StringRef path);
	Module *getModule(StringRef path);
	const Vector<StringRef> &getModuleStack();
	args::ArgParser &getCommandArgs();
	Context &getContext();

	// force ignores arg parser
	void dumpTokens(bool force);
	void dumpParseTree(bool force);
};
} // namespace sc

#endif // PARSER_HPP