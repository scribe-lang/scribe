/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

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
	Vector<StringRef> headerflags;
	Vector<StringRef> libflags;
	Vector<StringRef> preheadermacros;
	Vector<StringRef> headers;
	Vector<StringRef> macros;
	Vector<StringRef> typedefs;
	Vector<StringRef> structdecls;
	Vector<StringRef> funcdecls;
	struct ConstantInfo
	{
		// var: names of const declarations
		// decl: C code equivalent
		StringRef var;
		StringRef decl;
	};
	// constants: key is the constant data
	Map<StringRef, ConstantInfo> constants;

	StringRef getConstantDataVar(const lex::Lexeme &val, Type *ty);
	StringRef getNewConstantVar();
	static bool acceptsSemicolon(Stmt *stmt);
	bool getCTypeName(String &res, Stmt *stmt, Type *ty, bool for_cast, bool for_decl,
			  bool is_weak);
	bool getCValue(String &res, Stmt *stmt, Value *value, Type *type, bool i8_to_char = true);
	bool addStructDef(Stmt *stmt, StructTy *sty);
	bool writeCallArgs(const ModuleLoc *loc, const Vector<Stmt *> &args, Type *ty,
			   Writer &writer);
	bool applyCast(Stmt *stmt, Writer &writer, Writer &tmp);
	bool getFuncPointer(String &res, FuncTy *f, Stmt *stmt, bool for_cast, bool for_decl,
			    bool is_weak);
	StringRef getArrCount(Type *t);
	StringRef getSystemCompiler();

	inline StringRef getMangledName(StringRef name, Stmt *stmt)
	{
		String res = std::to_string(stmt->getValueTy(true)->getUniqID());
		res.insert(res.begin(), name.begin(), name.end());
		return ctx.moveStr(std::move(res));
	}
	inline StringRef getMangledName(StringRef name, Type *ty)
	{
		String res = std::to_string(ty->getUniqID());
		res.insert(res.begin(), name.begin(), name.end());
		return ctx.moveStr(std::move(res));
	}

public:
	CDriver(RAIIParser &parser);
	~CDriver() override;

	bool compile(StringRef outfile) override;

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
	bool visit(StmtFor *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtWhile *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtRet *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtContinue *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtBreak *stmt, Writer &writer, const bool &semicol);
	bool visit(StmtDefer *stmt, Writer &writer, const bool &semicol);
};
} // namespace sc

#endif // CODEGEN_C_HPP