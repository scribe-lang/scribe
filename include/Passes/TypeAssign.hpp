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

#ifndef PASSES_TYPE_ASSIGN_HPP
#define PASSES_TYPE_ASSIGN_HPP

#include "Passes/Base.hpp"
#include "TypeMgr.hpp"

namespace sc
{
class TypeAssignPass : public Pass
{
	TypeManager tmgr;

	std::string getMangledName(Stmt *stmt, const std::string &name,
				   ImportTy *import = nullptr) const;
	void applyPrimitiveTypeCoercion(Type *to, Stmt *from);
	void applyPrimitiveTypeCoercion(Stmt *lhs, Stmt *rhs, const lex::Lexeme &oper);
	bool chooseSuperiorPrimitiveType(Type *l, Type *r);
	bool initTemplateFunc(Stmt *caller, Type *&calledfn, const bool &has_va);

public:
	TypeAssignPass(ErrMgr &err, Context &ctx);
	~TypeAssignPass();

	bool visit(Stmt *stmt, Stmt **source);

	bool visit(StmtBlock *stmt, Stmt **source);
	bool visit(StmtType *stmt, Stmt **source);
	bool visit(StmtSimple *stmt, Stmt **source);
	bool visit(StmtFnCallInfo *stmt, Stmt **source);
	bool visit(StmtExpr *stmt, Stmt **source);
	bool visit(StmtVar *stmt, Stmt **source);
	bool visit(StmtFnSig *stmt, Stmt **source);
	bool visit(StmtFnDef *stmt, Stmt **source);
	bool visit(StmtHeader *stmt, Stmt **source);
	bool visit(StmtLib *stmt, Stmt **source);
	bool visit(StmtExtern *stmt, Stmt **source);
	bool visit(StmtEnum *stmt, Stmt **source);
	bool visit(StmtStruct *stmt, Stmt **source);
	bool visit(StmtVarDecl *stmt, Stmt **source);
	bool visit(StmtCond *stmt, Stmt **source);
	bool visit(StmtForIn *stmt, Stmt **source);
	bool visit(StmtFor *stmt, Stmt **source);
	bool visit(StmtWhile *stmt, Stmt **source);
	bool visit(StmtRet *stmt, Stmt **source);
	bool visit(StmtContinue *stmt, Stmt **source);
	bool visit(StmtBreak *stmt, Stmt **source);
};
} // namespace sc

#endif // PASSES_TYPE_ASSIGN_HPP