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

#include "Passes/Simplify.hpp"

#include "Parser.hpp"
#include "Utils.hpp"

namespace sc
{

static uint64_t genIntermediateID()
{
	static uint64_t id = 1;
	return id++;
}

SimplifyPass::SimplifyPass(ErrMgr &err, Context &ctx)
	: Pass(Pass::genPassID<SimplifyPass>(), err, ctx)
{}
SimplifyPass::~SimplifyPass() {}

bool SimplifyPass::visit(Stmt *stmt, Stmt **source)
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
	case FORIN: return visit(as<StmtForIn>(stmt), source);
	case FOR: return visit(as<StmtFor>(stmt), source);
	case WHILE: return visit(as<StmtWhile>(stmt), source);
	case RET: return visit(as<StmtRet>(stmt), source);
	case CONTINUE: return visit(as<StmtContinue>(stmt), source);
	case BREAK: return visit(as<StmtBreak>(stmt), source);
	case DEFER: return visit(as<StmtDefer>(stmt), source);
	}
	err.set(stmt, "invalid statement found for simplify pass: %s", stmt->getStmtTypeCString());
	return false;
}

bool SimplifyPass::visit(StmtBlock *stmt, Stmt **source)
{
	auto &stmts = stmt->getStmts();
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!visit(stmts[i], &stmts[i])) {
			err.set(stmt, "failed to perform simplify pass on stmt in block");
			return false;
		}
		if(!intermediates.empty()) {
			stmts.insert(stmts.begin() + i, intermediates.begin(), intermediates.end());
			i += intermediates.size();
			intermediates.clear();
		}
		if(!stmts[i]) {
			stmts.erase(stmts.begin() + i);
			--i;
			continue;
		}
		if(stmts[i]->getStmtType() == BLOCK &&
		   as<StmtBlock>(stmts[i])->getStmts().size() == 1) {
			StmtBlock *inner = as<StmtBlock>(stmts[i]);
			stmts.erase(stmts.begin() + i);
			stmts.insert(stmts.begin() + i, inner->getStmts().begin(),
				     inner->getStmts().end());
			i += inner->getStmts().size();
		}
	}
	return true;
}
bool SimplifyPass::visit(StmtType *stmt, Stmt **source)
{
	return true;
}
bool SimplifyPass::visit(StmtSimple *stmt, Stmt **source)
{
	return true;
}
bool SimplifyPass::visit(StmtFnCallInfo *stmt, Stmt **source)
{
	auto &args = stmt->getArgs();
	for(size_t i = 0; i < args.size(); ++i) {
		if(args[i]->getValue()->isType()) {
			args.erase(args.begin() + i);
			--i;
			continue;
		}
		if(!visit(args[i], &args[i])) {
			err.set(stmt, "failed to apply simplify pass on call argument");
			return false;
		}
	}
	return true;
}
bool SimplifyPass::visit(StmtExpr *stmt, Stmt **source)
{
	Stmt *&lhs	  = stmt->getLHS();
	Stmt *&rhs	  = stmt->getRHS();
	lex::TokType oper = stmt->getOper().getTokVal();
	if(lhs && !visit(lhs, &lhs)) {
		err.set(stmt, "failed to apply simplify pass on LHS in expression");
		return false;
	}
	if(rhs && !visit(rhs, &rhs)) {
		err.set(stmt, "failed to apply simplify pass on LHS in expression");
		return false;
	}
	if(!stmt->getCalledFn() || stmt->getValue()->hasPermaData()) return true;

	FuncTy *cf = stmt->getCalledFn();
	if(cf->isIntrinsic()) return true;

	if(oper == lex::FNCALL) {
		StmtFnCallInfo *ci = as<StmtFnCallInfo>(rhs);
		for(size_t i = 0; i < ci->getArgs().size(); ++i) {
			Stmt *a	   = ci->getArg(i);
			Stmt *newa = createIntermediate(cf, a, i);
			if(newa) ci->setArg(i, newa);
		}
	} else { // unary/binary operation
		Stmt *l	   = lhs;
		Stmt *newl = createIntermediate(cf, l, 0);
		if(newl) lhs = newl;
		if(rhs) {
			Stmt *r	   = rhs;
			Stmt *newr = createIntermediate(cf, r, 1);
			if(newr) rhs = newr;
		}
	}
	return true;
}
bool SimplifyPass::visit(StmtVar *stmt, Stmt **source)
{
	if(!stmt->getValueID()) {
		err.set(stmt, "variable: '%s' has no value id",
			stmt->getName().getDataStr().c_str());
		return false;
	}
	if(stmt->getValue()->isImport()) {
		*source = nullptr;
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->isFnDef()) {
		// set as entry point (main function) if signature matches
		static bool maindone = false;
		if(trySetMainFunction(stmt, stmt->getName().getDataStr())) {
			if(maindone) {
				err.set(stmt, "multiple main functions found");
				return false;
			}
			as<StmtFnDef>(stmt->getVVal())->incUsed();
			maindone = true;
			stmt->getName().setDataStr("main");
			stmt->setCodeGenMangle(true);
		}
	}
	bool had_val = stmt->getVVal();
	if(stmt->getVVal() && !visit(stmt->getVVal(), &stmt->getVVal())) {
		err.set(stmt, "failed to apply simplify pass on variable value expression");
		return false;
	}
	if(had_val && !stmt->getVVal()) {
		*source = nullptr;
		return true;
	}
	if(stmt->getVType() && !visit(stmt->getVType(), asStmt(&stmt->getVType()))) {
		err.set(stmt, "failed to apply simplify pass on variable type expression");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtFnSig *stmt, Stmt **source)
{
	if(!stmt->hasTemplatesDisabled()) {
		*source = nullptr;
		return true;
	}
	auto &args = stmt->getArgs();
	for(size_t i = 0; i < args.size(); ++i) {
		if(args[i]->getValueTy()->isVariadic()) {
			err.set(stmt, "variadic argument in function cannot reach simplify stage");
			return false;
		}
		Stmt *argtyexpr = args[i]->getVType()->getExpr();
		if(args[i]->getValue()->isType()) {
			args.erase(args.begin() + i);
			as<FuncTy>(stmt->getValueTy())->eraseArg(i);
			--i;
			continue;
		}
		if(!visit(args[i], asStmt(&args[i]))) {
			err.set(stmt, "failed to apply simplify pass on function signature arg");
			return false;
		}
		if(!args[i]) {
			args.erase(args.begin() + i);
			--i;
			continue;
		}
	}
	if(!visit(stmt->getRetType(), asStmt(&stmt->getRetType()))) {
		err.set(stmt, "failed to apply simplify pass on func signature ret type");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtFnDef *stmt, Stmt **source)
{
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err.set(stmt, "failed to apply simplify pass on func signature in definition");
		return false;
	}
	if(!stmt->getSig()) {
		*source = nullptr;
		return true;
	}
	if(!stmt->isUsed()) {
		stmt->decUsed();
		// No point in continuing as this will be erased in CleanupPass
		return true;
	}
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err.set(stmt, "failed to apply simplify pass on func def block");
		return false;
	}
	if(stmt->getBlk() && stmt->getBlk()->requiresTemplateInit()) {
		*source = nullptr;
		return true;
	}
	return true;
}
bool SimplifyPass::visit(StmtHeader *stmt, Stmt **source)
{
	return true;
}
bool SimplifyPass::visit(StmtLib *stmt, Stmt **source)
{
	return true;
}
bool SimplifyPass::visit(StmtExtern *stmt, Stmt **source)
{
	if(!stmt->getEntity()) return true;
	if(!visit(stmt->getEntity(), &stmt->getEntity())) {
		err.set(stmt, "failed to apply simplify pass on func signature in definition");
		return false;
	}
	if(!stmt->getEntity()) {
		*source = nullptr;
		return true;
	}
	if(stmt->getHeaders() && !visit(stmt->getHeaders(), asStmt(&stmt->getHeaders()))) {
		err.set(stmt, "failed to apply simplify pass on header in extern");
		return false;
	}
	if(stmt->getLibs() && !visit(stmt->getLibs(), asStmt(&stmt->getLibs()))) {
		err.set(stmt, "failed to apply simplify pass on libs in extern");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtEnum *stmt, Stmt **source)
{
	return true;
}
bool SimplifyPass::visit(StmtStruct *stmt, Stmt **source)
{
	// structs are not defined by themselves
	// they are defined when a struct type is encountered
	return true;
}
bool SimplifyPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(size_t i = 0; i < stmt->getDecls().size(); ++i) {
		auto &d = stmt->getDecls()[i];
		if(!visit(d, asStmt(&d))) {
			err.set(stmt, "failed to apply simplify pass on variable declaration");
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
bool SimplifyPass::visit(StmtCond *stmt, Stmt **source)
{
	for(auto &c : stmt->getConditionals()) {
		if(c.getCond() && !visit(c.getCond(), &c.getCond())) {
			err.set(stmt, "failed to apply simplify pass on conditional condition");
			return false;
		}
		if(c.getBlk() && !visit(c.getBlk(), asStmt(&c.getBlk()))) {
			err.set(stmt, "failed to apply simplify pass on conditional block");
			return false;
		}
	}
	return true;
}
bool SimplifyPass::visit(StmtForIn *stmt, Stmt **source)
{
	return true;
}
bool SimplifyPass::visit(StmtFor *stmt, Stmt **source)
{
	if(stmt->getInit() && !visit(stmt->getInit(), &stmt->getInit())) {
		err.set(stmt, "failed to apply simplify pass on for-loop init");
		return false;
	}
	if(stmt->getCond() && !visit(stmt->getCond(), &stmt->getCond())) {
		err.set(stmt, "failed to apply simplify pass on for-loop cond");
		return false;
	}
	if(stmt->getIncr() && !visit(stmt->getIncr(), &stmt->getIncr())) {
		err.set(stmt, "failed to apply simplify pass on for-loop incr");
		return false;
	}
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err.set(stmt, "failed to apply simplify pass on func def block");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtWhile *stmt, Stmt **source)
{
	if(stmt->getCond() && !visit(stmt->getCond(), &stmt->getCond())) {
		err.set(stmt, "failed to apply simplify pass on while condition");
		return false;
	}
	if(stmt->getBlk() && !visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err.set(stmt, "failed to apply simplify pass on while block");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtRet *stmt, Stmt **source)
{
	if(stmt->getVal() && !visit(stmt->getVal(), &stmt->getVal())) {
		err.set(stmt, "failed to apply simplify pass on return value");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtContinue *stmt, Stmt **source)
{
	return true;
}
bool SimplifyPass::visit(StmtBreak *stmt, Stmt **source)
{
	return true;
}
bool SimplifyPass::visit(StmtDefer *stmt, Stmt **source)
{
	return true;
}

bool SimplifyPass::trySetMainFunction(StmtVar *var, const std::string &varname)
{
	StmtFnDef *fn = as<StmtFnDef>(var->getVVal());
	if(!startsWith(var->getName().getDataStr(), "main_0")) return false;
	Type *retbase = fn->getSig()->getRetType()->getValueTy();
	if(!retbase || !retbase->isInt()) return false;
	// false => 0 args
	// true => 2 args
	bool zero_or_two = false;
	if(fn->getSigArgs().empty()) { // int main()
		return true;
	} else if(fn->getSigArgs().size() == 2) { // int main(int argc, char **argv)
		Type *a1 = fn->getSigArg(0)->getValueTy();
		Type *a2 = fn->getSigArg(1)->getValueTy();
		if(!a1->isInt()) return false;
		if(!a2->isPtr()) return false;
		if(!as<PtrTy>(a2)->getTo()->isPtr()) return false;
		if(!as<PtrTy>(as<PtrTy>(a2)->getTo())->getTo()->isInt()) return false;
		return true;
	}
	return false;
}
Stmt *SimplifyPass::createIntermediate(FuncTy *cf, Stmt *a, const size_t &i)
{
	if(!a->isExpr()) {
		return nullptr;
	}
	if(!cf->getArg(i)->hasRef()) {
		return nullptr;
	}
	StmtExpr *ax = as<StmtExpr>(a);
	if(!ax->getCalledFn()) return nullptr;
	std::string n;
	if(ax->getOper().getTokVal() == lex::FNCALL) {
		n = as<StmtSimple>(ax->getLHS())->getLexValue().getDataStr();
	} else {
		n = ax->getOper().getTok().getOperCStr();
	}
	n += "__interm" + std::to_string(genIntermediateID());
	lex::Lexeme name(a->getLoc(), lex::IDEN, n);
	StmtVar *v = StmtVar::create(ctx, a->getLoc(), name, nullptr, a, false, false, false);
	v->createAndSetValue(a->getValue()->clone(ctx));
	intermediates.push_back(v);
	StmtSimple *newa = StmtSimple::create(ctx, a->getLoc(), name);
	newa->setValueID(v);
	return newa;
}
} // namespace sc