/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef PARSER_HPP
#define PARSER_HPP

#include <cstddef>

#include "Args.hpp"
#include "Lex.hpp"
#include "Parser/Stmts.hpp"
// #include "parser/TypeMgr.hpp"
// #include "parser/ValueMgr.hpp"

namespace sc
{

class Module
{
	ErrMgr &err;

	std::string id;
	std::string path;
	std::string code;
	std::vector<lex::Lexeme> tokens;
	Stmt *ptree;

public:
	Module(ErrMgr &err, const std::string &id, const std::string &path,
	       const std::string &code);
	~Module();

	bool tokenize();
	bool parseTokens();
	// bool assignType(TypeMgr &types);
	// performs rearrangement of ptree to make all imports in order
	void rearrangeParseTree();
	void cleanupParseTree();

	const std::string &getID() const;
	const std::string &getPath() const;
	const std::string &getCode() const;
	const std::vector<lex::Lexeme> &getTokens() const;
	Stmt *&getParseTree();

	void dumpTokens() const;
	void dumpParseTree() const;
};

class RAIIParser
{
	args::ArgParser &args;

	ErrMgr err;

	// TypeMgr types;
	// ValueMgr values;

	// as new sources are imported, they'll be pushed back
	// the reverse iteration of this list will give the order of imports
	std::vector<std::string> modulestack;

	std::unordered_map<std::string, Module *> modules;

	Module *addModule(const std::string &path);

public:
	RAIIParser(args::ArgParser &args);
	~RAIIParser();

	bool hasModule(const std::string &path);
	Module *getModule(const std::string &path);
	const std::vector<std::string> &getModuleStack();

	bool parse(const std::string &path);
	void cleanupParseTrees();

	args::ArgParser &getCommandArgs();

	// force ignores arg parser
	void dumpTokens(const bool &force);
	void dumpParseTree(const bool &force);
};
} // namespace sc

#endif // PARSER_HPP