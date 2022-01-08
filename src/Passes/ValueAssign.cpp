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

#include "Passes/ValueAssign.hpp"

#include "Parser.hpp"
#include "Utils.hpp"

namespace sc
{
static bool break_stmt	  = false;
static bool continue_stmt = false;
static bool return_stmt	  = false;

ValueAssignPass::ValueAssignPass(Context &ctx) : Pass(Pass::genPassID<ValueAssignPass>(), ctx) {}
ValueAssignPass::~ValueAssignPass() {}

bool ValueAssignPass::visit(Stmt *stmt, Stmt **source)
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
	case WHILE: return visit(as<StmtWhile>(stmt), source);
	case RET: return visit(as<StmtRet>(stmt), source);
	case CONTINUE: return visit(as<StmtContinue>(stmt), source);
	case BREAK: return visit(as<StmtBreak>(stmt), source);
	case DEFER: return visit(as<StmtDefer>(stmt), source);
	}
	err::out(stmt,
		 {"invalid statement found for type assignment: ", stmt->getStmtTypeCString()});
	return false;
}

bool ValueAssignPass::visit(StmtBlock *stmt, Stmt **source)
{
	auto &stmts = stmt->getStmts();
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!visit(stmts[i], &stmts[i])) {
			err::out(stmt, {"failed to assign type to stmt in block"});
			return false;
		}
		if(break_stmt || continue_stmt || return_stmt) break;
	}
	return true;
}
bool ValueAssignPass::visit(StmtType *stmt, Stmt **source)
{
	return true;
}
bool ValueAssignPass::visit(StmtSimple *stmt, Stmt **source)
{
	lex::Lexeme &tok = stmt->getLexValue();
	switch(stmt->getLexValue().getTokVal()) {
	case lex::TRUE:	 // fallthrough
	case lex::FALSE: // fallthrough
	case lex::NIL:	 // fallthrough
	case lex::INT:	 // fallthrough
	case lex::FLT:	 // fallthrough
	case lex::CHAR:	 // fallthrough
	case lex::STR:	 // fallthrough
	case lex::I1:	 // fallthrough
	case lex::I8:	 // fallthrough
	case lex::I16:	 // fallthrough
	case lex::I32:	 // fallthrough
	case lex::I64:	 // fallthrough
	case lex::U8:	 // fallthrough
	case lex::U16:	 // fallthrough
	case lex::U32:	 // fallthrough
	case lex::U64:	 // fallthrough
	case lex::F32:	 // fallthrough
	case lex::F64:	 // fallthrough
	case lex::IDEN:
		if(!stmt->getValue()->hasData() && stmt->getDecl() &&
		   !visit(stmt->getDecl(), asStmt(&stmt->getDecl())))
		{
			err::out(stmt, {"failed to determine value from declaration"});
			return false;
		}
		break;
	default: {
		err::out(stmt, {"cannot assign value - unknown simple type"});
		return false;
	}
	}
	return true;
}
bool ValueAssignPass::visit(StmtFnCallInfo *stmt, Stmt **source)
{
	for(auto &a : stmt->getArgs()) {
		if(!visit(a, asStmt(&a))) {
			err::out(stmt, {"failed to determine type of argument"});
			return false;
		}
	}
	return true;
}
bool ValueAssignPass::visit(StmtExpr *stmt, Stmt **source)
{
	Stmt *&lhs	  = stmt->getLHS();
	Stmt *&rhs	  = stmt->getRHS();
	lex::TokType oper = stmt->getOper().getTokVal();

	if(oper != lex::FNCALL && oper != lex::STCALL && lhs && !visit(lhs, &lhs)) {
		err::out(stmt, {"failed to determine value of LHS in expression with operation: ",
				stmt->getOper().getTok().getOperCStr()});
		return false;
	}

	if((oper == lex::DOT || oper == lex::ARROW)) goto skip_rhs_val;
	if(rhs && !rhs->isFnCallInfo() && !visit(rhs, &rhs)) {
		err::out(stmt, {"failed to determine value of RHS in expression with operation: ",
				stmt->getOper().getTok().getOperCStr()});
		return false;
	}

skip_rhs_val:
	switch(oper) {
	case lex::ARROW: // fallthrough
	case lex::DOT: {
		// nothing to be done for dot operation since all that is required
		// is already done in type assign pass
		StructVal *sv	    = as<StructVal>(lhs->getValue());
		StmtSimple *rsim    = as<StmtSimple>(rhs);
		StringRef fieldname = rsim->getLexValue().getDataStr();
		// struct field names are not mangled with the module ids
		Value *res = sv->getField(fieldname);
		if(res && stmt->getValue() != res) stmt->changeValue(res);
		break;
	}
	case lex::FNCALL: {
		StmtFnCallInfo *callinfo = as<StmtFnCallInfo>(rhs);
		bool has_va		 = false;
		Vector<Stmt *> &callargs = callinfo->getArgs();
		for(auto &a : callargs) {
			if(!visit(a, &a)) {
				err::out(stmt, {"failed to determine value of arg in struct call"});
				return false;
			}
		}
		FuncTy *fn = as<FuncTy>(lhs->getValueTy());
		if(fn->isIntrinsic()) {
			if(!fn->isParseIntrinsic() &&
			   !fn->callIntrinsic(ctx, stmt, source, callargs)) {
				err::out(stmt, {"failed to call value intrinsic"});
				return false;
			}
			return true;
		}
		if(!fn->getVar()) {
			err::out(stmt, {"function type contains no definition"});
			return false;
		}
		Stmt *&fndef			 = fn->getVar()->getVVal();
		StmtFnDef *def			 = as<StmtFnDef>(fndef);
		const Vector<StmtVar *> &defargs = def->getSigArgs();
		if(!fndef) {
			err::out(stmt, {"function has no definition to execute"});
			return false;
		}
		if(defargs.size() != callargs.size()) {
			err::out(stmt,
				 {"function definition and call must have same argument len (def: ",
				  ctx.strFrom(defargs.size()),
				  ", call: ", ctx.strFrom(callargs.size()), ")"});
			return false;
		}
		for(size_t i = 0; i < defargs.size(); ++i) {
			Value *aval = callargs[i]->getValue();
			def->getSigArg(i)->updateValue(ctx, aval);
		}
		if(!visit(fndef, &fndef)) {
			err::out(stmt, {"failed to determine value from function definition"});
			return false;
		}
		// update the callee's arguments if they are references
		for(size_t i = 0; i < defargs.size(); ++i) {
			Value *aval = callargs[i]->getValue();
			if(def->getSigArg(i)->getValueTy()->hasRef()) {
				aval->updateValue(ctx, defargs[i]->getValue());
			}
		}

		stmt->updateValue(ctx, def->getBlk()->getValue());
		def->clearValue();
		return_stmt = false;
		break;
	}
	case lex::STCALL: {
		StmtFnCallInfo *callinfo = as<StmtFnCallInfo>(rhs);
		Vector<Stmt *> &callargs = callinfo->getArgs();
		for(auto &a : callargs) {
			if(!visit(a, &a)) {
				err::out(stmt, {"failed to determine value of arg in struct call"});
				return false;
			}
		}
		stmt->getValue()->setContainsData();
		break;
	}
	// address of
	case lex::UAND: // fallthrough
	// dereference
	case lex::UMUL: {
		break;
	}
	case lex::SUBS: {
		if(!lhs->getValueTy()->isPtr()) goto applyoperfn;
		assert(lhs->getValue()->isVec() && "value of pointer/array must be a vector");
		VecVal *vaval = as<VecVal>(lhs->getValue());
		IntVal *iv    = as<IntVal>(rhs->getValue());
		if(vaval->getVal().size() <= iv->getVal()) {
			err::out(stmt, {"index out of bounds of pointer/array"});
			return false;
		}
		stmt->changeValue(vaval->getValAt(iv->getVal()));
		break;
	}
	case lex::ASSN:
	// Arithmetic
	case lex::ADD:
	case lex::SUB:
	case lex::MUL:
	case lex::DIV:
	case lex::MOD:
	case lex::ADD_ASSN:
	case lex::SUB_ASSN:
	case lex::MUL_ASSN:
	case lex::DIV_ASSN:
	case lex::MOD_ASSN:
	// Post/Pre Inc/Dec
	case lex::XINC:
	case lex::INCX:
	case lex::XDEC:
	case lex::DECX:
	// Unary
	case lex::UADD:
	case lex::USUB:
	// Logic
	case lex::LAND:
	case lex::LOR:
	case lex::LNOT:
	// Comparison
	case lex::EQ:
	case lex::LT:
	case lex::GT:
	case lex::LE:
	case lex::GE:
	case lex::NE:
	// Bitwise
	case lex::BAND:
	case lex::BOR:
	case lex::BNOT:
	case lex::BXOR:
	case lex::BAND_ASSN:
	case lex::BOR_ASSN:
	case lex::BNOT_ASSN:
	case lex::BXOR_ASSN:
	// Others
	case lex::LSHIFT:
	case lex::RSHIFT:
	case lex::LSHIFT_ASSN:
	case lex::RSHIFT_ASSN: {
	applyoperfn:
		Vector<Stmt *> args = {lhs};
		if(rhs) args.push_back(rhs);
		for(auto &a : args) {
			if(!visit(a, &a)) {
				err::out(stmt, {"failed to determine value of arg in struct call"});
				return false;
			}
		}
		FuncTy *fn = stmt->getCalledFn();
		if(fn->isIntrinsic()) {
			if(!fn->isParseIntrinsic() && !fn->callIntrinsic(ctx, stmt, source, args)) {
				err::out(stmt, {"failed to call value intrinsic"});
				return false;
			}
			return true;
		}
		Stmt *&fndef			 = fn->getVar()->getVVal();
		StmtFnDef *def			 = as<StmtFnDef>(fndef);
		const Vector<StmtVar *> &defargs = def->getSigArgs();
		if(!fndef) {
			err::out(stmt, {"function has no definition to execute"});
			return false;
		}
		if(def->getSigArgs().size() != args.size()) {
			err::out(stmt, {"function def and call must have same argument count"});
			return false;
		}
		for(size_t i = 0; i < defargs.size(); ++i) {
			Value *aval = args[i]->getValue();
			def->getSigArg(i)->updateValue(ctx, aval);
		}
		if(!visit(fndef, &fndef)) {
			err::out(stmt, {"failed to determine value from function definition"});
			return false;
		}
		// update the callee's arguments if they are references
		for(size_t i = 0; i < defargs.size(); ++i) {
			Value *aval = args[i]->getValue();
			if(def->getSigArg(i)->getValueTy()->hasRef()) {
				aval->updateValue(ctx, def->getSigArg(i)->getValue());
			}
		}
		stmt->updateValue(ctx, def->getBlk()->getValue());
		def->clearValue();
		return_stmt = false;
		break;
	}
	default: err::out(stmt->getOper(), {"nonexistent operator"}); return false;
	}
	return true;
}
bool ValueAssignPass::visit(StmtVar *stmt, Stmt **source)
{
	Stmt *&val = stmt->getVVal();
	if(!val) return true;
	if(val->getStmtType() == FNDEF) return true;

	if(!visit(val, &val)) {
		err::out(stmt, {"failed to determine value for variable"});
		return false;
	}
	stmt->updateValue(ctx, val->getValue());
	return true;
}
bool ValueAssignPass::visit(StmtFnSig *stmt, Stmt **source)
{
	// handled in StmtExpr
	return true;
}
bool ValueAssignPass::visit(StmtFnDef *stmt, Stmt **source)
{
	StmtBlock *&blk = stmt->getBlk();
	if(!blk) {
		err::out(stmt, {"failed to get value from a function definition without body"});
		return false;
	}
	if(!visit(blk, asStmt(&blk))) {
		err::out(stmt, {"failed to determine value from function definition block"});
		return false;
	}
	stmt->updateValue(ctx, blk->getValue());
	return true;
}
bool ValueAssignPass::visit(StmtHeader *stmt, Stmt **source)
{
	return true;
}
bool ValueAssignPass::visit(StmtLib *stmt, Stmt **source)
{
	return true;
}
bool ValueAssignPass::visit(StmtExtern *stmt, Stmt **source)
{
	return true;
}
bool ValueAssignPass::visit(StmtEnum *stmt, Stmt **source)
{
	return true;
}
bool ValueAssignPass::visit(StmtStruct *stmt, Stmt **source)
{
	return true;
}
bool ValueAssignPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(auto &d : stmt->getDecls()) {
		if(!visit(d, asStmt(&d))) {
			err::out(stmt, {"failed to determine value of this variable declaration"});
			return false;
		}
	}
	return true;
}
bool ValueAssignPass::visit(StmtCond *stmt, Stmt **source)
{
	if(stmt->isInline()) {
		if(stmt->getConditionals().empty()) return true;
		StmtBlock *&blk = stmt->getConditionals()[0].getBlk();
		if(!visit(blk, asStmt(&blk))) {
			err::out(stmt, {"failed to determine value for inline conditional block"});
			return false;
		}
		return true;
	}
	for(auto &c : stmt->getConditionals()) {
		Stmt *&cond	= c.getCond();
		StmtBlock *&blk = c.getBlk();
		if(!cond) {
			if(!visit(blk, asStmt(&blk))) {
				err::out(stmt, {"failed to determine else-block value"});
				return false;
			}
			break;
		}
		if(!visit(cond, &cond)) {
			err::out(stmt, {"failed to determine conditional value"});
			return false;
		}
		if(!(cond->getValue()->isInt() && as<IntVal>(cond->getValue())->getVal()) &&
		   !(cond->getValue()->isFlt() && as<FltVal>(cond->getValue())->getVal()))
		{
			continue;
		}
		if(!visit(blk, asStmt(&blk))) {
			err::out(stmt, {"failed to determine conditional-block value"});
			return false;
		}
		break;
	}
	return true;
}
bool ValueAssignPass::visit(StmtFor *stmt, Stmt **source)
{
	Stmt *&init	= stmt->getInit();
	Stmt *&cond	= stmt->getCond();
	Stmt *&incr	= stmt->getIncr();
	StmtBlock *&blk = stmt->getBlk();
	if(stmt->isInline()) {
		if(init && !visit(init, &init)) {
			err::out(stmt, {"failed to determine init value for inline for loop"});
			return false;
		}
		if(!visit(blk, asStmt(&blk))) {
			err::out(stmt, {"failed to determine block value for inline for loop"});
			return false;
		}
		stmt->clearValue();
		return true;
	}
	if(init && !visit(init, &init)) {
		err::out(stmt, {"failed to determine value for for-init statement"});
		return false;
	}
	if(!cond) {
		err::out(stmt, {"condition must be present in for loop for value assignment pass"});
		return false;
	}
	if(!visit(cond, &cond)) {
		err::out(stmt, {"failed to determine value for for-condition expression"});
		return false;
	}
	while((cond->getValue()->isInt() && as<IntVal>(cond->getValue())->getVal()) ||
	      (cond->getValue()->isFlt() && as<FltVal>(cond->getValue())->getVal()))
	{
		if(!visit(blk, asStmt(&blk))) {
			err::out(stmt, {"failed to determine value for for-loop block"});
			return false;
		}
		continue_stmt = false;
		if(break_stmt) break;
		if(incr) incr->clearValue();
		if(incr && !visit(incr, &incr)) {
			err::out(stmt, {"failed to determine incr value for for-loop"});
			return false;
		}
		cond->clearValue();
		if(!visit(cond, &cond)) {
			err::out(stmt, {"failed to determine value for for-condition expression"});
			return false;
		}
	}
	break_stmt = false;
	return true;
}
bool ValueAssignPass::visit(StmtWhile *stmt, Stmt **source)
{
	Stmt *&cond	= stmt->getCond();
	StmtBlock *&blk = stmt->getBlk();
	if(!visit(cond, &cond)) {
		err::out(stmt, {"failed to determine value for for-condition expression"});
		return false;
	}
	while((cond->getValue()->isInt() && as<IntVal>(cond->getValue())->getVal()) ||
	      (cond->getValue()->isFlt() && as<FltVal>(cond->getValue())->getVal()))
	{
		if(!visit(blk, asStmt(&blk))) {
			err::out(stmt, {"failed to determine value for for-loop block"});
			return false;
		}
		continue_stmt = false;
		if(break_stmt) break;
		cond->clearValue();
		if(!visit(cond, &cond)) {
			err::out(stmt, {"failed to determine value for for-condition expression"});
			return false;
		}
	}
	break_stmt = false;
	return true;
}
bool ValueAssignPass::visit(StmtRet *stmt, Stmt **source)
{
	Stmt *val = stmt->getVal();
	if(val && !visit(val, &val)) {
		err::out(stmt, {"failed to determine value of return argument"});
		return false;
	}
	// nothing to do for VoidVal
	if(!val) return true;
	stmt->updateValue(ctx, val->getValue());
	return_stmt = true;
	return true;
}
bool ValueAssignPass::visit(StmtContinue *stmt, Stmt **source)
{
	continue_stmt = true;
	return true;
}
bool ValueAssignPass::visit(StmtBreak *stmt, Stmt **source)
{
	break_stmt = true;
	return true;
}
bool ValueAssignPass::visit(StmtDefer *stmt, Stmt **source)
{
	return true;
}
} // namespace sc