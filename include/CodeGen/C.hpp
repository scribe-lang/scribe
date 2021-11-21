/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef CODEGEN_C_HPP
#define CODEGEN_C_HPP

#include "Base.hpp"
#include "Writer.hpp"

namespace sc
{
class CDriver : public CodeGenDriver
{
	std::vector<std::string> headerflags;
	std::vector<std::string> libflags;
	std::vector<std::string> headers;
	std::vector<std::string> macros;
	std::vector<std::string> typedefs;
	std::vector<std::string> structdecls;
	std::vector<std::string> funcdecls;
	struct ConstantInfo
	{
		// var: names of const declarations
		// decl: C code equivalent
		std::string var;
		std::string decl;
	};
	// constants: key is the constant data
	std::unordered_map<std::string, ConstantInfo> constants;

	const std::string &getConstantDataVar(const lex::Lexeme &val, Type *ty);
	std::string getNewConstantVar();
	static bool acceptsSemicolon(Stmt *stmt);
	bool trySetMainFunction(StmtVar *var, const std::string &varname, Writer &writer);
	bool getCTypeName(std::string &res, Stmt *stmt, Type *ty, bool arr_as_ptr);
	bool getCValue(std::string &res, Stmt *stmt, Value *value, Type *type);
	bool addStructDef(Stmt *stmt, StructTy *sty);

	inline std::string getMangledName(const std::string &name, Stmt *stmt)
	{
		return name + std::to_string(stmt->getValueTy(true)->getID());
	}

public:
	CDriver(RAIIParser &parser);
	~CDriver();

	bool compile(const std::string &outfile, const bool &ir_only);

	bool visit(Stmt *stmt, Writer &writer, const bool &semicol);

	bool visit(StmtBlock *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtType *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtSimple *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtFnCallInfo *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtExpr *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtVar *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtFnSig *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtFnDef *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtHeader *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtLib *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtExtern *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtEnum *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtStruct *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtVarDecl *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtCond *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtForIn *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtFor *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtWhile *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtRet *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtContinue *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtBreak *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtDefer *stmt, Writer &writer, const bool &semicol);
};
} // namespace sc

#endif // CODEGEN_C_HPP