#include "AST/Passes/IRGen.hpp"

namespace sc::AST
{

IRGenPass::IRGenPass() : Pass(Pass::genPassID<IRGenPass>()) {}
IRGenPass::~IRGenPass() {}

bool IRGenPass::visit(Stmt *stmt, Stmt **source)
{
	bool res = false;
	switch(stmt->getStmtType()) {
	case BLOCK: res = visit(as<StmtBlock>(stmt), source); break;
	case TYPE: res = visit(as<StmtType>(stmt), source); break;
	case SIMPLE: res = visit(as<StmtSimple>(stmt), source); break;
	case EXPR: res = visit(as<StmtExpr>(stmt), source); break;
	case CALLARGS: res = visit(as<StmtCallArgs>(stmt), source); break;
	case VAR: res = visit(as<StmtVar>(stmt), source); break;
	case SIGNATURE: res = visit(as<StmtSignature>(stmt), source); break;
	case FNDEF: res = visit(as<StmtFnDef>(stmt), source); break;
	case VARDECL: res = visit(as<StmtVarDecl>(stmt), source); break;
	case COND: res = visit(as<StmtCond>(stmt), source); break;
	case FOR: res = visit(as<StmtFor>(stmt), source); break;
	case ONEWORD: res = visit(as<StmtOneWord>(stmt), source); break;
	default: {
		err::out(stmt, "invalid statement found for IR gen: ", stmt->getStmtTypeCString());
		break;
	}
	}
	return res;
}

bool IRGenPass::visit(StmtBlock *stmt, Stmt **source)
{
	for(auto &s : stmt->getStmts()) {
		if(!visit(s, &s)) {
			err::out(s, "failed to generate code for statement");
			return false;
		}
	}
	return true;
}
bool IRGenPass::visit(StmtType *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtSimple *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtCallArgs *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtExpr *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtVar *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtSignature *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtFnDef *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtVarDecl *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtCond *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtFor *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtOneWord *stmt, Stmt **source) { return true; }

} // namespace sc::AST