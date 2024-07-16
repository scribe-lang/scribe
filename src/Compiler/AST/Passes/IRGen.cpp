#include "AST/Passes/IRGen.hpp"

namespace sc::AST
{
IRGenPass::IRGenPass(Context &ctx) : Pass(Pass::genPassID<IRGenPass>(), ctx) {}
IRGenPass::~IRGenPass() {}

bool IRGenPass::visit(Stmt *stmt, Stmt **source) { return true; }

bool IRGenPass::visit(StmtBlock *stmt, Stmt **source) { return true; }
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