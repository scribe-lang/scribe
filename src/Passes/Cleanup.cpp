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

#include "Passes/Cleanup.hpp"

#include "Parser.hpp"
#include "Utils.hpp"

namespace sc
{
CleanupPass::CleanupPass(Context &ctx) : Pass(Pass::genPassID<CleanupPass>(), ctx) {}
CleanupPass::~CleanupPass() {}

bool CleanupPass::visit(Stmt *stmt, Stmt **source)
{
	switch(stmt->getStmtType()) {
	case BLOCK: return visit(as<StmtBlock>(stmt), source);
	case TYPE: return visit(as<StmtType>(stmt), source);
	case SIMPLE: return visit(as<StmtSimple>(stmt), source);
	case EXPR: return visit(as<StmtExpr>(stmt), source);
	case FNCALLINFO: return visit(as<StmtFnCallInfo>(stmt), source);
	case VAR: return visit(as<StmtVar>(stmt), source);
	case FNSIG: return visit(as<StmtFnSig>(stmt), source);
	case FNDEF: return visit(as<StmtFnDef>(stmt), source);
	case HEADER: return visit(as<StmtHeader>(stmt), source);
	case LIB: return visit(as<StmtLib>(stmt), source);
	case EXTERN: return visit(as<StmtExtern>(stmt), source);
	case ENUMDEF: return visit(as<StmtEnum>(stmt), source);
	case STRUCTDEF: return visit(as<StmtStruct>(stmt), source);
	case VARDECL: return visit(as<StmtVarDecl>(stmt), source);
	case COND: return visit(as<StmtCond>(stmt), source);
	case FOR: return visit(as<StmtFor>(stmt), source);
	case RET: return visit(as<StmtRet>(stmt), source);
	case CONTINUE: return visit(as<StmtContinue>(stmt), source);
	case BREAK: return visit(as<StmtBreak>(stmt), source);
	case DEFER: return visit(as<StmtDefer>(stmt), source);
	}
	err::out(stmt, {"invalid statement found for cleanup pass: ", stmt->getStmtTypeCString()});
	return false;
}

bool CleanupPass::visit(StmtBlock *stmt, Stmt **source)
{
	auto &stmts = stmt->getStmts();
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!visit(stmts[i], &stmts[i])) {
			err::out(stmt, {"failed to perform cleanup of stmt in block"});
			return false;
		}
		if(!stmts[i]) {
			stmts.erase(stmts.begin() + i);
			--i;
			continue;
		}
	}
	return true;
}
bool CleanupPass::visit(StmtType *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtSimple *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtFnCallInfo *stmt, Stmt **source)
{
	auto &args = stmt->getArgs();
	for(size_t i = 0; i < args.size(); ++i) {
		if(!visit(args[i], &args[i])) {
			err::out(stmt, {"failed to apply cleanup pass on call argument"});
			return false;
		}
	}
	return true;
}
bool CleanupPass::visit(StmtExpr *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtVar *stmt, Stmt **source)
{
	// As such, a function will never be non-unique in signature.
	// This is because functions can have same signature and name,
	// and still be different.
	// (say, a function with single 'type' parameter, returning void)
	// Therefore, there is no point in checking function's uniqueness
	// based on its name and/or ID.

	bool had_val = stmt->getVVal();
	if(stmt->getVVal() && !visit(stmt->getVVal(), &stmt->getVVal())) {
		err::out(stmt, {"failed to apply cleanup pass on variable value expression"});
		return false;
	}
	if(had_val && !stmt->getVVal()) {
		*source = nullptr;
		return true;
	}

	return true;
}
bool CleanupPass::visit(StmtFnSig *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtFnDef *stmt, Stmt **source)
{
	if(!stmt->isUsed()) {
		*source = nullptr;
		return true;
	}
	return true;
}
bool CleanupPass::visit(StmtHeader *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtLib *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtExtern *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtEnum *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtStruct *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(size_t i = 0; i < stmt->getDecls().size(); ++i) {
		auto &d = stmt->getDecls()[i];
		if(!visit(d, asStmt(&d))) {
			err::out(stmt, {"failed to apply cleanup pass on variable declaration"});
			return false;
		}
		if(!d) {
			stmt->getDecls().erase(stmt->getDecls().begin() + i);
			--i;
		}
	}
	if(stmt->getDecls().empty()) {
		*source = nullptr;
		return true;
	}
	return true;
}
bool CleanupPass::visit(StmtCond *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtFor *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtRet *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtContinue *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtBreak *stmt, Stmt **source) { return true; }
bool CleanupPass::visit(StmtDefer *stmt, Stmt **source) { return true; }
} // namespace sc