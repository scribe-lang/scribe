#pragma once

#include "Base.hpp"

namespace sc
{

class Value;

namespace ast
{

class IRGenPass : public Pass
{
	Allocator &allocator;
	Vector<Value *> &ir;

public:
	IRGenPass(Allocator &allocator, Vector<Value *> &ir);
	~IRGenPass();

	bool visit(Stmt *stmt, Stmt **source) override;

	bool visit(StmtBlock *stmt, Stmt **source) override;
	bool visit(StmtType *stmt, Stmt **source) override;
	bool visit(StmtSimple *stmt, Stmt **source) override;
	bool visit(StmtCallArgs *stmt, Stmt **source) override;
	bool visit(StmtExpr *stmt, Stmt **source) override;
	bool visit(StmtVar *stmt, Stmt **source) override;
	bool visit(StmtSignature *stmt, Stmt **source) override;
	bool visit(StmtFnDef *stmt, Stmt **source) override;
	bool visit(StmtVarDecl *stmt, Stmt **source) override;
	bool visit(StmtCond *stmt, Stmt **source) override;
	bool visit(StmtFor *stmt, Stmt **source) override;
	bool visit(StmtOneWord *stmt, Stmt **source) override;
};

} // namespace ast
} // namespace sc