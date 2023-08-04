#include "Passes/Simplify.hpp"

#include "Utils.hpp"

namespace sc
{

static uint64_t genIntermediateID()
{
	static uint64_t id = 1;
	return id++;
}

SimplifyPass::SimplifyPass(Context &ctx) : Pass(Pass::genPassID<SimplifyPass>(), ctx) {}
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
	case FOR: return visit(as<StmtFor>(stmt), source);
	case RET: return visit(as<StmtRet>(stmt), source);
	case CONTINUE: return visit(as<StmtContinue>(stmt), source);
	case BREAK: return visit(as<StmtBreak>(stmt), source);
	case DEFER: return visit(as<StmtDefer>(stmt), source);
	}
	err::out(stmt, "invalid statement found for simplify pass: ", stmt->getStmtTypeCString());
	return false;
}

bool SimplifyPass::visit(StmtBlock *stmt, Stmt **source)
{
	auto &stmts = stmt->getStmts();
	intermediates.push_back({});
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!visit(stmts[i], &stmts[i])) {
			err::out(stmt, "failed to perform simplify pass on stmt in block");
			return false;
		}
		auto &intermediate_list = intermediates.back();
		if(!intermediate_list.empty()) {
			stmts.insert(stmts.begin() + i, intermediate_list.begin(),
				     intermediate_list.end());
			i += intermediate_list.size();
			intermediate_list.clear();
		}
		if(!stmts[i]) {
			stmts.erase(stmts.begin() + i);
			--i;
			continue;
		}
		if(stmts[i]->getStmtType() == BLOCK && stmt->isTop()) {
			StmtBlock *inner = as<StmtBlock>(stmts[i]);
			stmts.erase(stmts.begin() + i);
			stmts.insert(stmts.begin() + i, inner->getStmts().begin(),
				     inner->getStmts().end());
			i += inner->getStmts().size();
			--i;
		}
	}
	intermediates.pop_back();
	return true;
}
bool SimplifyPass::visit(StmtType *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtSimple *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtFnCallInfo *stmt, Stmt **source)
{
	auto &args = stmt->getArgs();
	for(size_t i = 0; i < args.size(); ++i) {
		if(args[i]->getVal() && args[i]->getVal()->isType()) {
			args.erase(args.begin() + i);
			--i;
			continue;
		}
		if(!visit(args[i], &args[i])) {
			err::out(stmt, "failed to apply simplify pass on call argument");
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
		err::out(stmt, "failed to apply simplify pass on LHS in expression");
		return false;
	}
	if(rhs && !visit(rhs, &rhs)) {
		err::out(stmt, "failed to apply simplify pass on LHS in expression");
		return false;
	}
	if(!stmt->getCalledFn() || stmt->getVal()) return true;

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
	if(stmt->getVal() && stmt->getVal()->isNamespace()) {
		*source = nullptr;
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->isFnDef()) {
		// set as entry point (main function) if signature matches
		static bool maindone = false;
		if(trySetMainFunction(stmt, stmt->getName().getDataStr())) {
			if(maindone) {
				err::out(stmt, "multiple main functions found");
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
		err::out(stmt, "failed to apply simplify pass on variable value expression");
		return false;
	}
	if(had_val && !stmt->getVVal()) {
		*source = nullptr;
		return true;
	}
	if(stmt->getVType() && !visit(stmt->getVType(), asStmt(&stmt->getVType()))) {
		err::out(stmt, "failed to apply simplify pass on variable type expression");
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
		if(args[i]->getTy()->isVariadic()) {
			err::out(stmt, "variadic argument in function cannot reach simplify stage");
			return false;
		}
		Stmt *argtyexpr = args[i]->getVType()->getExpr();
		if(args[i]->getVal() && args[i]->getVal()->isType()) {
			args.erase(args.begin() + i);
			as<FuncTy>(stmt->getTy())->eraseArg(i);
			--i;
			continue;
		}
		if(!visit(args[i], asStmt(&args[i]))) {
			err::out(stmt, "failed to apply simplify pass on function signature arg");
			return false;
		}
		if(!args[i]) {
			args.erase(args.begin() + i);
			--i;
			continue;
		}
	}
	if(!visit(stmt->getRetType(), asStmt(&stmt->getRetType()))) {
		err::out(stmt, "failed to apply simplify pass on func signature ret type");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtFnDef *stmt, Stmt **source)
{
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err::out(stmt, "failed to apply simplify pass on func signature in definition");
		return false;
	}
	if(!stmt->getSig()) {
		*source = nullptr;
		return true;
	}
	if(stmt->getBlk() && stmt->getBlk()->requiresTemplateInit()) {
		*source = nullptr;
		return true;
	}
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err::out(stmt, "failed to apply simplify pass on func def block");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtHeader *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtLib *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtExtern *stmt, Stmt **source)
{
	if(!stmt->getEntity()) return true;
	if(!visit(stmt->getEntity(), &stmt->getEntity())) {
		err::out(stmt, "failed to apply simplify pass on func signature in definition");
		return false;
	}
	if(!stmt->getEntity()) {
		*source = nullptr;
		return true;
	}
	if(stmt->getHeaders() && !visit(stmt->getHeaders(), asStmt(&stmt->getHeaders()))) {
		err::out(stmt, "failed to apply simplify pass on header in extern");
		return false;
	}
	if(stmt->getLibs() && !visit(stmt->getLibs(), asStmt(&stmt->getLibs()))) {
		err::out(stmt, "failed to apply simplify pass on libs in extern");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtEnum *stmt, Stmt **source) { return true; }
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
			err::out(stmt, "failed to apply simplify pass on variable declaration");
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
			err::out(stmt, "failed to apply simplify pass on conditional condition");
			return false;
		}
		if(c.getBlk() && !visit(c.getBlk(), asStmt(&c.getBlk()))) {
			err::out(stmt, "failed to apply simplify pass on conditional block");
			return false;
		}
	}
	return true;
}
bool SimplifyPass::visit(StmtFor *stmt, Stmt **source)
{
	if(stmt->getInit() && !visit(stmt->getInit(), &stmt->getInit())) {
		err::out(stmt, "failed to apply simplify pass on for-loop init");
		return false;
	}
	if(stmt->getCond() && !visit(stmt->getCond(), &stmt->getCond())) {
		err::out(stmt, "failed to apply simplify pass on for-loop cond");
		return false;
	}
	if(stmt->getIncr() && !visit(stmt->getIncr(), &stmt->getIncr())) {
		err::out(stmt, "failed to apply simplify pass on for-loop incr");
		return false;
	}
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err::out(stmt, "failed to apply simplify pass on func def block");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtRet *stmt, Stmt **source)
{
	if(stmt->getRetVal() && !visit(stmt->getRetVal(), &stmt->getRetVal())) {
		err::out(stmt, "failed to apply simplify pass on return value");
		return false;
	}
	return true;
}
bool SimplifyPass::visit(StmtContinue *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtBreak *stmt, Stmt **source) { return true; }
bool SimplifyPass::visit(StmtDefer *stmt, Stmt **source) { return true; }

bool SimplifyPass::trySetMainFunction(StmtVar *var, StringRef varname)
{
	StmtFnDef *fn = as<StmtFnDef>(var->getVVal());
	if(!startsWith(var->getName().getDataStr(), "main_")) return false;
	Type *retbase = fn->getSig()->getRetType()->getTy();
	if(!retbase || !retbase->isInt()) return false;
	// false => 0 args
	// true => 2 args
	bool zero_or_two = false;
	if(fn->getSigArgs().empty()) { // int main()
		return true;
	} else if(fn->getSigArgs().size() == 2) { // int main(int argc, char **argv)
		Type *a1 = fn->getSigArg(0)->getTy();
		Type *a2 = fn->getSigArg(1)->getTy();
		if(!a1->isInt()) return false;
		if(!a2->isPtr()) return false;
		if(!as<PtrTy>(a2)->getTo()->isPtr()) return false;
		if(!as<PtrTy>(as<PtrTy>(a2)->getTo())->getTo()->isInt()) return false;
		return true;
	}
	return false;
}
Stmt *SimplifyPass::createIntermediate(FuncTy *cf, Stmt *a, size_t i)
{
	if(!a->isExpr()) {
		return nullptr;
	}
	if(cf->getSig() && !cf->getSig()->getArg(i)->isRef()) {
		return nullptr;
	}
	StmtExpr *ax = as<StmtExpr>(a);
	if(!ax->getCalledFn()) return nullptr;
	StringRef n;
	if(ax->getOper().getTokVal() == lex::FNCALL) {
		n = as<StmtSimple>(ax->getLHS())->getLexValue().getDataStr();
	} else {
		n = ax->getOper().getTok().getOperCStr();
	}
	n = ctx.strFrom({n, "__interm", ctx.strFrom(genIntermediateID())});
	lex::Lexeme name(a->getLoc(), lex::IDEN, n);
	StmtVar *v = StmtVar::create(ctx, a->getLoc(), name, nullptr, a, 0);
	v->setTyVal(a);
	v->appendStmtMask(a->getStmtMask());
	a->castTo(nullptr, 0);
	intermediates.back().push_back(v);
	StmtSimple *newa = StmtSimple::create(ctx, a->getLoc(), name);
	newa->setTyVal(v);
	newa->appendStmtMask(a->getStmtMask());
	return newa;
}
} // namespace sc