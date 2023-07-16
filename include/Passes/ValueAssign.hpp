#pragma once

#include "Passes/Base.hpp"

namespace sc
{
class ValueAssignPass : public Pass
{
public:
	ValueAssignPass(Context &ctx);
	~ValueAssignPass() override;

	bool visit(Stmt *stmt, Stmt **source) override;

	bool visit(StmtBlock *stmt, Stmt **source) override;
	bool visit(StmtType *stmt, Stmt **source) override;
	bool visit(StmtSimple *stmt, Stmt **source) override;
	bool visit(StmtFnCallInfo *stmt, Stmt **source) override;
	bool visit(StmtExpr *stmt, Stmt **source) override;
	bool visit(StmtVar *stmt, Stmt **source) override;
	bool visit(StmtFnSig *stmt, Stmt **source) override;
	bool visit(StmtFnDef *stmt, Stmt **source) override;
	bool visit(StmtHeader *stmt, Stmt **source) override;
	bool visit(StmtLib *stmt, Stmt **source) override;
	bool visit(StmtExtern *stmt, Stmt **source) override;
	bool visit(StmtEnum *stmt, Stmt **source) override;
	bool visit(StmtStruct *stmt, Stmt **source) override;
	bool visit(StmtVarDecl *stmt, Stmt **source) override;
	bool visit(StmtCond *stmt, Stmt **source) override;
	bool visit(StmtFor *stmt, Stmt **source) override;
	bool visit(StmtRet *stmt, Stmt **source) override;
	bool visit(StmtContinue *stmt, Stmt **source) override;
	bool visit(StmtBreak *stmt, Stmt **source) override;
	bool visit(StmtDefer *stmt, Stmt **source) override;
};
} // namespace sc
