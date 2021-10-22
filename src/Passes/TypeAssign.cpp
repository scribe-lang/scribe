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

#include "Passes/TypeAssign.hpp"

#include "Parser.hpp"
#include "Utils.hpp"

namespace sc
{
TypeAssignPass::TypeAssignPass(ErrMgr &err, Context &ctx)
	: Pass(Pass::genPassID<TypeAssignPass>(), err, ctx), tmgr(ctx), vpass(err, ctx),
	  disabled_varname_mangling(false)
{}
TypeAssignPass::~TypeAssignPass() {}

bool TypeAssignPass::visit(Stmt *stmt, Stmt **source)
{
	bool res = false;
	switch(stmt->getStmtType()) {
	case BLOCK: res = visit(as<StmtBlock>(stmt), source); break;
	case TYPE: res = visit(as<StmtType>(stmt), source); break;
	case SIMPLE: res = visit(as<StmtSimple>(stmt), source); break;
	case EXPR: res = visit(as<StmtExpr>(stmt), source); break;
	case FNCALLINFO: res = visit(as<StmtFnCallInfo>(stmt), source); break;
	case VAR: res = visit(as<StmtVar>(stmt), source); break;
	case FNSIG: res = visit(as<StmtFnSig>(stmt), source); break;
	case FNDEF: res = visit(as<StmtFnDef>(stmt), source); break;
	case HEADER: res = visit(as<StmtHeader>(stmt), source); break;
	case LIB: res = visit(as<StmtLib>(stmt), source); break;
	case EXTERN: res = visit(as<StmtExtern>(stmt), source); break;
	case ENUMDEF: res = visit(as<StmtEnum>(stmt), source); break;
	case STRUCTDEF: res = visit(as<StmtStruct>(stmt), source); break;
	case VARDECL: res = visit(as<StmtVarDecl>(stmt), source); break;
	case COND: res = visit(as<StmtCond>(stmt), source); break;
	case FORIN: res = visit(as<StmtForIn>(stmt), source); break;
	case FOR: res = visit(as<StmtFor>(stmt), source); break;
	case WHILE: res = visit(as<StmtWhile>(stmt), source); break;
	case RET: res = visit(as<StmtRet>(stmt), source); break;
	case CONTINUE: res = visit(as<StmtContinue>(stmt), source); break;
	case BREAK: res = visit(as<StmtBreak>(stmt), source); break;
	case DEFER: res = visit(as<StmtDefer>(stmt), source); break;
	default: {
		err.set(stmt, "invalid statement found for type assignment: %s",
			stmt->getStmtTypeCString());
		break;
	}
	}
	if(!res) return false;
	if(!source || !*source) return res;
	stmt = *source;
	if(stmt && stmt->getType() && stmt->getType()->hasComptime() &&
	   !stmt->getType()->isTemplate()) {
		if(!vpass.visit(stmt, source)) {
			err.set(stmt, "failed to get value for a comptime type");
			return false;
		}
	}
	return res;
}

bool TypeAssignPass::visit(StmtBlock *stmt, Stmt **source)
{
	if(stmt->getMod()->isMainModule() || !stmt->isTop()) tmgr.pushLayer();
	auto &stmts = stmt->getStmts();
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!visit(stmts[i], &stmts[i])) {
			err.set(stmt, "failed to assign type to stmt in block");
			return false;
		}
		if(!stmts[i]) {
			stmts.erase(stmts.begin() + i);
			--i;
			continue;
		}
	}
	if(stmt->getMod()->isMainModule() || !stmt->isTop()) tmgr.popLayer();
	// insert all the specfns
	if(stmt->isTop()) {
		stmts.insert(stmts.begin(), specfns.begin(), specfns.end());
		specfns.clear();
	}
	return true;
}
bool TypeAssignPass::visit(StmtType *stmt, Stmt **source)
{
	if(!visit(stmt->getExpr(), &stmt->getExpr()) || !stmt->getExpr()->getType()) {
		err.set(stmt, "failed to determine type of type-expr");
		return false;
	}
	Type *res = stmt->getExpr()->getType();
	if(res->isTypeTy() && as<TypeTy>(res)->getContainedTy()) {
		res = as<TypeTy>(res)->getContainedTy()->clone(ctx);
	} else {
		res = res->clone(ctx);
	}
	res->setInfo(stmt->getInfoMask());

	// generate ptrs
	for(size_t i = 0; i < stmt->getPtrCount(); ++i) {
		res = PtrTy::create(ctx, res, 0);
	}
	stmt->setType(res);
	return true;
}
bool TypeAssignPass::visit(StmtSimple *stmt, Stmt **source)
{
	switch(stmt->getLexValue().getTok().getVal()) {
	case lex::VOID: stmt->setType(VoidTy::create(ctx)); break;
	case lex::ANY: stmt->setType(AnyTy::create(ctx)); break;
	case lex::TYPE: stmt->setType(TypeTy::create(ctx)); break;
	case lex::TRUE:	 // fallthrough
	case lex::FALSE: // fallthrough
	case lex::NIL: stmt->setType(IntTy::create(ctx, 1, true)); break;
	case lex::INT: stmt->setType(IntTy::create(ctx, 32, 1)); break;
	case lex::FLT: stmt->setType(FltTy::create(ctx, 32)); break;
	case lex::CHAR: stmt->setType(IntTy::create(ctx, 8, 1)); break;
	case lex::STR: {
		Type *ty = IntTy::create(ctx, 8, 1);
		ty->setConst();
		ty = PtrTy::create(ctx, ty, stmt->getLexValue().getDataStr().size());
		stmt->setType(ty);
		break;
	}
	case lex::IDEN: {
		StmtVar *decl		 = nullptr;
		Module *mod		 = stmt->getMod();
		std::string name	 = stmt->getLexValue().getDataStr();
		std::string mangled_name = getMangledName(stmt, name);
		if(!stmt->isAppliedModuleID()) {
			stmt->setType(tmgr.getTy(mangled_name, false, true));
			decl = tmgr.getDecl(mangled_name, false, true);
			if(stmt->getType()) stmt->updateLexDataStr(mangled_name);
		}
		if(!stmt->getType()) {
			stmt->setType(tmgr.getTy(name, false, true));
			decl = tmgr.getDecl(name, false, true);
		}
		stmt->setAppliedModuleID(true);
		stmt->setDecl(decl);
		break;
	}
	default: return false;
	}
	if(!stmt->getType()) {
		err.set(stmt, "undefined variable: %s", stmt->getLexValue().getDataStr().c_str());
		return false;
	}
	return true;
}
bool TypeAssignPass::visit(StmtFnCallInfo *stmt, Stmt **source)
{
	tmgr.pushLayer();
	for(auto &a : stmt->getArgs()) {
		if(!visit(a, asStmt(&a))) {
			err.set(stmt, "failed to determine type of argument");
			return false;
		}
	}
	tmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtExpr *stmt, Stmt **source)
{
	if(stmt->getLHS() && !visit(stmt->getLHS(), &stmt->getLHS())) {
		err.set(stmt, "failed to determine type of LHS in expression");
		return false;
	}
	lex::TokType oper = stmt->getOper().getTok().getVal();
	if(oper != lex::DOT && oper != lex::ARROW && stmt->getRHS() &&
	   !visit(stmt->getRHS(), &stmt->getRHS()))
	{
		err.set(stmt, "failed to determine type of RHS in expression");
		return false;
	}
	Stmt *&lhs = stmt->getLHS();
	Stmt *&rhs = stmt->getRHS();
	switch(oper) {
	case lex::ARROW: {
		if(!lhs->getType()->isPtr()) {
			err.set(lhs, "LHS must be a pointer for arrow access");
			return false;
		}
	}
	case lex::DOT: {
		assert(rhs && rhs->getStmtType() == SIMPLE &&
		       "RHS stmt type for dot operation must be simple");
		StmtSimple *rsim = as<StmtSimple>(rhs);
		if(lhs->getType()->isImport()) {
			ImportTy *import = as<ImportTy>(lhs->getType());
			std::string mangled =
			getMangledName(rhs, rsim->getLexValue().getDataStr(), import);
			rsim->updateLexDataStr(mangled);
			rsim->setAppliedModuleID(true);
			if(!visit(stmt->getRHS(), &stmt->getRHS())) {
				err.set(stmt, "failed to determine type of RHS in dot expression");
				return false;
			}
			// replace this stmt with RHS (effectively removing LHS - import)
			*source	       = rhs;
			stmt->getRHS() = nullptr;
			delete stmt;
			// once the source is changed, stmt will also be invalidated - therefore,
			// cannot be used anymore
			return true;
		}
		Type *res		     = nullptr;
		const std::string &fieldname = rsim->getLexValue().getDataStr();
		if(lhs->getType()->isStruct()) {
			StructTy *lst = as<StructTy>(lhs->getType());
			if(lst->isDef()) {
				err.set(stmt, "cannot use dot operator on a structure definition; "
					      "instantiate the struct first");
				return false;
			}
			// struct field names are not mangled with the module ids
			res = lst->getField(fieldname);
			if(res) {
				rhs->setType(res);
				stmt->setType(res);
				break;
			}
		}
		if(!(res = tmgr.getTypeFn(lhs->getType(), fieldname))) {
			err.set(stmt, "no field or function '%s' in type '%s'", fieldname.c_str(),
				lhs->getType()->toStr().c_str());
			return false;
		}
		// erase the expression, replace with the rhs alone
		rsim->setSelf(lhs);
		rsim->setType(res);
		stmt->getLHS() = nullptr;
		stmt->getRHS() = nullptr;
		*source	       = rsim;
		delete stmt;
		// can't do anything after source modification since stmt not of same type
		return true;
	}
	case lex::FNCALL: {
		assert(rhs && rhs->getStmtType() == FNCALLINFO &&
		       "RHS must be function call info for a function call");
		if(!lhs->getType()->isFunc() && !lhs->getType()->isStruct()) {
			err.set(stmt, "func call can be performed only on funcs or struct defs");
			return false;
		}
		StmtFnCallInfo *callinfo  = as<StmtFnCallInfo>(rhs);
		std::vector<Stmt *> &args = callinfo->getArgs();
		if(lhs->getStmtType() == SIMPLE && as<StmtSimple>(lhs)->getSelf()) {
			args.insert(args.begin(), as<StmtSimple>(lhs)->getSelf());
		}
		if(lhs->getType()->isFunc()) {
			FuncTy *fn  = as<FuncTy>(lhs->getType());
			bool has_va = fn->hasVariadic();
			if(!(fn = fn->createCall(ctx, err, stmt->getLoc(), args))) {
				err.set(stmt, "function is incompatible with call arguments");
				return false;
			}
			lhs->setType(fn);
			stmt->setType(fn->getRet());
			size_t fnarglen	  = fn->getArgs().size();
			size_t callarglen = args.size();
			for(size_t i = 0, j = 0, k = 0; i < fnarglen && j < callarglen; ++i, ++j) {
				Type *coerced_to = fn->getArg(i);
				Stmt *&arg	 = args[j];
				if(coerced_to->isVariadic()) {
					coerced_to = as<VariadicTy>(coerced_to)->getArg(k++);
					--i;
				}
				applyPrimitiveTypeCoercion(coerced_to, arg);
				if(!coerced_to->hasComptime() || vpass.visit(args[j], &args[j])) {
					continue;
				}
				err.set(stmt, "failed to determine value for comptime arg");
				return false;
			}
			if(stmt->isIntrinsicCall()) {
				if(!fn->isIntrinsic()) {
					err.set(stmt, "function call is intrinsic but the function "
						      "itself is not");
					return false;
				}
				if(fn->isIntrinsicParse() &&
				   !fn->callIntrinsic(ctx, err, stmt, source, args, IPARSE)) {
					err.set(stmt, "call to parse intrinsic failed");
					return false;
				}
				stmt->setCalledFnTy(fn);
			} else if(fn->isIntrinsic()) {
				err.set(stmt, "function is intrinsic - required '@' before call");
				return false;
			}
			// apply template specialization
			if(!initTemplateFunc(stmt, fn, args)) return false;
		} else if(lhs->getType()->isStruct()) {
			StructTy *st = as<StructTy>(lhs->getType());
			if(!st->isDef()) {
				err.set(stmt, "only struct definitions can be instantiated");
				return false;
			}
			if(!(st = st->instantiate(ctx, err, stmt->getLoc(), callinfo->getArgs()))) {
				err.set(stmt, "failed to instantiate struct with given arguments");
				return false;
			}
			size_t fnarglen	  = st->getFields().size();
			size_t callarglen = callinfo->getArgs().size();
			for(size_t i = 0, j = 0, k = 0; i < fnarglen && j < callarglen; ++i, ++j) {
				Type *coerced_to = st->getField(i);
				if(coerced_to->isVariadic()) {
					coerced_to = as<VariadicTy>(coerced_to)->getArg(k++);
					--i;
				}
				applyPrimitiveTypeCoercion(coerced_to, callinfo->getArg(j));
			}
			lhs->setType(st);
			stmt->setType(st);
		}
		break;
	}
	case lex::SUBS: {
		if(lhs->getType()->isVariadic()) {
			if(lhs->getStmtType() != SIMPLE) {
				err.set(stmt, "LHS in variadic subscript must be a simple stmt");
				return false;
			}
			if(!rhs->getType()->isIntegral()) {
				err.set(rhs, "index for a variadic must be integral");
				return false;
			}
			if(!vpass.visit(rhs, &rhs) || !rhs->getValue()) {
				err.set(stmt, "variadic index must be calculable at comptime");
				return false;
			}
			if(rhs->getValue()->getType() != VINT) {
				err.set(rhs, "variadic index must be an integer");
				return false;
			}
			IntVal *iv     = as<IntVal>(rhs->getValue());
			VariadicTy *va = as<VariadicTy>(lhs->getType());
			if(va->getArgs().size() <= iv->getVal()) {
				err.set(stmt,
					"variadic index out of bounds "
					"(va: %zu, index: %" PRId64 ")",
					va->getArgs().size(), iv->getVal());
				return false;
			}
			stmt->setType(va->getArg(iv->getVal()));
			break;
		} else if(lhs->getType()->isPtr()) {
			if(!rhs->getType()->isIntegral()) {
				err.set(rhs, "index for a pointer must be integral");
				return false;
			}
			stmt->setType(as<PtrTy>(lhs->getType())->getTo());
			break;
		}
		goto applyoperfn;
	}
	// address of
	case lex::UAND: {
		stmt->setType(PtrTy::create(ctx, lhs->getType(), 0));
		break;
	}
	// dereference
	case lex::UMUL: {
		if(!lhs->getType()->isPtr()) {
			err.set(stmt, "cannot dereference non pointer type: %s",
				lhs->getType()->toStr().c_str());
			return false;
		}
		stmt->setType(as<PtrTy>(lhs->getType())->getTo());
		break;
	}
	case lex::ASSN: {
		Stmt *store = lhs;
		lhs	    = rhs;
		rhs	    = store;
		goto applyoperfn;
	}
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
		applyPrimitiveTypeCoercion(lhs, rhs, stmt->getOper());
		lex::Tok &optok = stmt->getOper().getTok();
		FuncTy *fn	= tmgr.getTypeFn(lhs->getType(), optok.getOperCStr());
		if(!fn) {
			err.set(stmt, "function '%s' does not exist for type: %s",
				optok.getOperCStr(), lhs->getType()->toStr().c_str());
			return false;
		}

		std::vector<Stmt *> args = {lhs};
		if(rhs) args.push_back(rhs);
		if(!(fn = fn->createCall(ctx, err, stmt->getLoc(), args))) {
			err.set(stmt, "function is incompatible with call arguments");
			return false;
		}
		stmt->setType(fn->getRet());

		for(size_t i = 0; i < args.size(); ++i) {
			Type *coerced_to = fn->getArg(i);
			Stmt *&arg	 = args[i];
			if(!coerced_to->hasComptime() || vpass.visit(args[i], &args[i])) {
				continue;
			}
			err.set(stmt, "failed to determine value for comptime arg");
			return false;
		}

		if(fn->isIntrinsic()) {
			if(fn->isIntrinsicParse() &&
			   !fn->callIntrinsic(ctx, err, stmt, source, args, IPARSE)) {
				err.set(stmt, "call to parse intrinsic failed");
				return false;
			}
			stmt->setCalledFnTy(fn);
		}
		if(!initTemplateFunc(stmt, fn, args)) {
			err.set(stmt, "failed to intialize template function");
			return false;
		}
		break;
	}
	default: err.set(stmt->getOper(), "nonexistent operator"); return false;
	}
	if(stmt->getCommas() > 0) {
		stmt->setType(nullptr);
	}
	return true;
}
bool TypeAssignPass::visit(StmtVar *stmt, Stmt **source)
{
	Stmt *&val	 = stmt->getVVal();
	StmtType *&vtype = stmt->getVType();
	Type *inty	 = nullptr;
	if(val && val->getStmtType() == FNDEF) {
		as<StmtFnDef>(val)->setParentVar(stmt);
	}
	if(stmt->isGlobal()) goto post_mangling;
	if(val && val->getStmtType() == FNDEF && stmt->isIn()) {
		goto post_mangling;
	}
	if(disabled_varname_mangling || stmt->isAppliedModuleID()) goto post_mangling;
	stmt->getName().setDataStr(getMangledName(stmt, stmt->getName().getDataStr()));
	stmt->setAppliedModuleID(true);
post_mangling:
	if(val && (!visit(val, &val) || !val->getType())) {
		err.set(stmt, "unable to determine type of value of this variable");
		return false;
	}
	if(vtype && (!visit(vtype, asStmt(&vtype)) || !vtype->getType())) {
		err.set(stmt, "unable to determine type from the given type of this variable");
		return false;
	}
	if((!val || val->getStmtType() != FNDEF) &&
	   tmgr.exists(stmt->getName().getDataStr(), true, false)) {
		err.set(stmt->getName(), "variable '%s' already exists in scope",
			stmt->getName().getDataStr().c_str());
		return false;
	}
	if(val && val->getType()->isVoid()) {
		err.set(stmt, "value expression returns void, which cannot be assigned to a var");
		return false;
	}
	if(vtype && val &&
	   !vtype->getType()->isCompatible(ctx, val->getType(), err, stmt->getLoc())) {
		err.set(stmt, "incompatible given type and value of the variable decl");
		return false;
	}
	if(val && !vtype) {
		stmt->setType(val->getType());
	} else if(vtype) {
		stmt->setType(vtype->getType());
	}

	if(stmt->isComptime()) {
		if(val && (!vpass.visit(val, &val) || !val->getValue())) {
			err.set(stmt, "value of comptime variable could not be calculated");
			return false;
		}
		stmt->getType()->setComptime();
	}
	if(val) stmt->setVal(val->getValue());

	if((!val || val->getStmtType() != STRUCTDEF) && stmt->getType()->isStruct()) {
		as<StructTy>(stmt->getType())->setDef(false);
	}
	if(vtype && val) applyPrimitiveTypeCoercion(vtype->getType(), val);
	if(stmt->isIn()) {
		StmtFnDef *def = as<StmtFnDef>(stmt->getVVal());
		StmtVar *self  = def->getSigArgs()[0];
		return tmgr.addTypeFn(self->getType(), stmt->getName().getDataStr(),
				      as<FuncTy>(stmt->getType()));
	}
	return tmgr.addVar(stmt->getName().getDataStr(), stmt->getType(), stmt, stmt->isGlobal());
}
bool TypeAssignPass::visit(StmtFnSig *stmt, Stmt **source)
{
	auto &args		  = stmt->getArgs();
	disabled_varname_mangling = true;
	for(size_t i = 0; i < args.size(); ++i) {
		if(!visit(args[i], asStmt(&args[i]))) {
			err.set(stmt, "failed to determine type of argument");
			return false;
		}
	}
	if(!visit(stmt->getRetType(), asStmt(&stmt->getRetType()))) {
		err.set(stmt, "failed to determine type of return type");
		return false;
	}
	disabled_varname_mangling = false;
	std::vector<Type *> argst;
	for(auto &a : args) {
		argst.push_back(a->getType());
	}
	stmt->setScope(tmgr.getCurrentLayerIndex() - 1);
	Type *retty = stmt->getRetType()->getType();
	stmt->setType(FuncTy::create(ctx, nullptr, argst, retty, nullptr, INONE, false));
	return true;
}
bool TypeAssignPass::visit(StmtFnDef *stmt, Stmt **source)
{
	tmgr.pushLayer();
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err.set(stmt, "failed to determine type of func signature");
		return false;
	}
	FuncTy *sigty = as<FuncTy>(stmt->getSig()->getType());

	sigty->setVar(stmt->getParentVar());

	if(stmt->requiresTemplateInit()) goto end;

	tmgr.pushFunc(sigty);
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err.set(stmt, "failed to determine type of function block");
		return false;
	}
	tmgr.popFunc();
end:
	stmt->setType(sigty);
	tmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtHeader *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtLib *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtExtern *stmt, Stmt **source)
{
	tmgr.pushLayer();
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err.set(stmt, "failed to determine type of func signature");
		return false;
	}
	if(stmt->getHeaders() && !visit(stmt->getHeaders(), asStmt(&stmt->getHeaders()))) {
		err.set(stmt, "failed to assign header type");
		return false;
	}
	if(stmt->getLibs() && !visit(stmt->getLibs(), asStmt(&stmt->getLibs()))) {
		err.set(stmt, "failed to assign header type");
		return false;
	}
	stmt->setType(stmt->getSig()->getType());
	tmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtEnum *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtStruct *stmt, Stmt **source)
{
	tmgr.pushLayer();
	std::vector<std::string> fieldnames;
	std::vector<Type *> fieldtypes;
	disabled_varname_mangling = true;
	for(auto &f : stmt->getFields()) {
		if(!visit(f, asStmt(&f))) {
			err.set(stmt, "failed to determine type of struct field");
			return false;
		}
		fieldnames.push_back(f->getName().getDataStr());
		fieldtypes.push_back(f->getType());
	}
	disabled_varname_mangling = false;

	StructTy *st = StructTy::create(ctx, fieldnames, fieldtypes);
	st->setDef(true);
	stmt->setType(st);
	tmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(auto &d : stmt->getDecls()) {
		if(!visit(d, asStmt(&d))) {
			err.set(stmt, "failed to determine type of this variable declaration");
			return false;
		}
	}
	return true;
}
bool TypeAssignPass::visit(StmtCond *stmt, Stmt **source)
{
	for(auto &cond : stmt->getConditionals()) {
		Stmt *&c	= cond.getCond();
		StmtBlock *&b	= cond.getBlk();
		bool this_is_it = false;
		if(!c) goto after_cond_check;
		if(!visit(c, &c)) {
			err.set(stmt, "failed to determine type of conditional");
			return false;
		}
		if(!c->getType()->isPrimitive()) {
			err.set(stmt, "conditional expression type must be primitive");
			return false;
		}
		if(!stmt->isInline()) {
			if(!visit(b, asStmt(&b))) {
				err.set(stmt, "failed to determine types"
					      " in inline conditional block");
				return false;
			}
			continue;
		}
		if(!vpass.visit(c, &c)) {
			err.set(stmt, "failed to get condition value for inline conditional");
			return false;
		}
		if(c->getValue()->getType() == VINT) {
			this_is_it = as<IntVal>(c->getValue())->getVal() != 0;
		}
		if(c->getValue()->getType() == VFLT) {
			this_is_it = as<FltVal>(c->getValue())->getVal() != 0.0;
		}
		if(!this_is_it) continue;
	after_cond_check:
		if(!b) continue;
		if(!visit(b, asStmt(&b))) {
			err.set(stmt, "failed to determine types in inline conditional block");
			return false;
		}
		Stmt *res = b;
		b	  = nullptr;
		delete stmt;
		*source = res;
		return true;
	}
	// reached here && conditional is inline = delete the entire conditional
	if(stmt->isInline()) {
		delete stmt;
		*source = nullptr;
	}
	return true;
}
bool TypeAssignPass::visit(StmtForIn *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtFor *stmt, Stmt **source)
{
	if(stmt->isInline() && !stmt->getCond()) {
		err.set(stmt, "inline for-loop requires a condition");
		return false;
	}

	Stmt *&init	    = stmt->getInit();
	Stmt *&cond	    = stmt->getCond();
	Stmt *&incr	    = stmt->getIncr();
	StmtBlock *&blk	    = stmt->getBlk();
	StmtBlock *finalblk = nullptr;
	std::vector<Stmt *> newblkstmts;

	tmgr.pushLayer();
	if(init && !visit(init, &init)) {
		err.set(stmt, "failed to determine type of init expression in for loop");
		return false;
	}
	if(cond && !visit(cond, &cond)) {
		err.set(stmt, "failed to determine type of cond expression in for loop");
		return false;
	}
	if(incr && !visit(incr, &incr)) {
		err.set(stmt, "failed to determine type of incr expression in for loop");
		return false;
	}
	if(!cond->getType()->isPrimitive()) {
		err.set(stmt, "inline for-loop's condition must be a primitive (int/flt)");
		return false;
	}

	if(!stmt->isInline() && blk && !visit(blk, asStmt(&blk))) {
		err.set(stmt, "failed to determine type of for-loop block");
		return false;
	}
	if(!stmt->isInline()) {
		tmgr.popLayer();
		return true;
	}

	if(!blk) {
		*source = nullptr;
		delete stmt;
		return true;
	}

	if(init && !vpass.visit(init, &init)) {
		err.set(stmt, "failed to determine value of inline for-loop init expr");
		return false;
	}
	if(!vpass.visit(cond, &cond)) {
		err.set(stmt, "failed to determine value of inline for-loop condition");
		return false;
	}
	if(init) newblkstmts.push_back(init->clone());
	while((cond->getType()->isInt() && as<IntVal>(cond->getValue())->getVal()) ||
	      (cond->getType()->isFlt() && as<FltVal>(cond->getValue())->getVal()))
	{
		for(auto &s : blk->getStmts()) {
			newblkstmts.push_back(s->clone());
		}
		if(incr) newblkstmts.push_back(incr->clone());
		if(incr && !vpass.visit(incr, &incr)) {
			err.set(stmt, "failed to determine value of inline for-loop incr");
			return false;
		}
		if(!vpass.visit(cond, &cond)) {
			err.set(stmt, "failed to determine value of inline for-loop condition");
			return false;
		}
	}
	for(auto &s : blk->getStmts()) delete s;
	blk->getStmts() = newblkstmts;
	finalblk	= blk;
	stmt->getBlk()	= nullptr;
	delete stmt;
	*source = finalblk;
	tmgr.popLayer();
	if(!visit(*source, source)) {
		err.set(*source, "failed to determine type of inlined for-loop block");
		return false;
	}
	return true;
}
bool TypeAssignPass::visit(StmtWhile *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtRet *stmt, Stmt **source)
{
	if(!tmgr.hasFunc()) {
		err.set(stmt, "return statements can be in functions only");
		return false;
	}
	Stmt *&val = stmt->getVal();
	if(val && !visit(val, &val)) {
		err.set(stmt, "failed to determine type of the return argument");
		return false;
	}
	Type *valtype = val ? val->getType() : VoidTy::create(ctx);
	FuncTy *fn    = tmgr.getTopFunc();
	if(!fn->getVar()) {
		err.set(stmt, "function type has no declaration");
		return false;
	}
	Type *fnretty = fn->getRet();
	if(!fnretty->isCompatible(ctx, valtype, err, stmt->getLoc())) {
		err.set(stmt,
			"function return type and deduced return type are"
			" incompatible (function return type: %s, deduced: %s)",
			fnretty->toStr().c_str(), valtype->toStr().c_str());
		return false;
	}
	stmt->setType(valtype);
	stmt->setFnBlk(as<StmtFnDef>(fn->getVar()->getVVal())->getBlk());
	return true;
}
bool TypeAssignPass::visit(StmtContinue *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtBreak *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtDefer *stmt, Stmt **source)
{
	if(!visit(stmt->getVal(), &stmt->getVal())) {
		err.set(stmt, "failed to determine type of defer expression");
		return false;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// Extra ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

std::string TypeAssignPass::getMangledName(Stmt *stmt, const std::string &name,
					   ImportTy *import) const
{
	if(stmt->getStmtType() == SIMPLE) {
		StmtSimple *sim = as<StmtSimple>(stmt);
		if(sim->isAppliedModuleID()) return name;
	}
	return name + "_" + (import ? import->getImportID() : stmt->getMod()->getID());
}

void TypeAssignPass::applyPrimitiveTypeCoercion(Type *to, Stmt *from)
{
	if(!to || !from) return;
	if(!to->isPrimitive() || !from->getType()->isPrimitive()) return;

	if(to->getID() == from->getType()->getID()) return;
	from->castTo(to);
}

void TypeAssignPass::applyPrimitiveTypeCoercion(Stmt *lhs, Stmt *rhs, const lex::Lexeme &oper)
{
	if(!lhs || !rhs) return;
	if(!lhs->getType()->isPrimitive() || !rhs->getType()->isPrimitive()) return;
	if(oper.getTok().getVal() == lex::SUBS) return;

	Type *l = lhs->getType();
	Type *r = rhs->getType();

	if(l->getID() == r->getID()) return;

	if(oper.getTok().isAssign()) {
		rhs->castTo(l);
		return;
	}
	// 0 => lhs
	// 1 => rhs
	bool superior = chooseSuperiorPrimitiveType(l, r);
	if(superior) {
		rhs->castTo(l);
	} else {
		lhs->castTo(r);
	}
}

bool TypeAssignPass::chooseSuperiorPrimitiveType(Type *l, Type *r)
{
	assert(l->isPrimitive() && r->isPrimitive() &&
	       "superior type can be chosen between primitives only");

	if(l->isFlt() && r->isInt()) return true;
	if(r->isFlt() && l->isInt()) return false;

	if(l->isFlt() && r->isFlt()) {
		if(as<FltTy>(l)->getBits() > as<FltTy>(r)->getBits()) return true;
		return false;
	}
	if(l->isInt() && r->isInt()) {
		if(as<IntTy>(l)->getBits() > as<IntTy>(r)->getBits()) return true;
		if(as<IntTy>(l)->getBits() < as<IntTy>(r)->getBits()) return false;
		if(!as<IntTy>(l)->isSigned() && as<IntTy>(r)->isSigned()) return true;
		if(as<IntTy>(l)->isSigned() && !as<IntTy>(r)->isSigned()) return false;
	}
	return true;
}

bool TypeAssignPass::initTemplateFunc(Stmt *caller, Type *calledfn, std::vector<Stmt *> &args)
{
	assert(calledfn && "LHS has no type assigned");
	// nothing to do if function has no definition
	if(!calledfn->isFunc()) return true;
	FuncTy *cf = as<FuncTy>(calledfn);
	if(!cf->getVar() || !cf->getVar()->getVVal()) return true;
	StmtVar *&cfvar = cf->getVar();
	if(!cfvar) return true;
	StmtFnDef *cfdef = as<StmtFnDef>(cfvar->getVVal());
	cfdef->setParentVar(cfvar);
	if(!cfdef->requiresTemplateInit()) return true;
	cfvar = as<StmtVar>(cfvar->clone()); // template must be cloned
	cf->setVar(cfvar);
	Pointer<StmtVar> cfvarptr(cfvar); // must be unset if everything goes right
	cfdef		  = as<StmtFnDef>(cfvar->getVVal());
	StmtFnSig *&cfsig = cfdef->getSig();
	StmtBlock *&cfblk = cfdef->getBlk();
	cfsig->setVariadic(false);
	if(!cfblk) {
		err.set(caller, "function definition for specialization has no block");
		return false;
	}
	if(caller->getMod()->getID() == cfdef->getMod()->getID()) {
		tmgr.lockScopeBelow(cfsig->getScope());
	}

	tmgr.pushLayer();

	for(size_t i = 0; i < cf->getArgs().size(); ++i) {
		StmtVar *cfa = cfsig->getArgs()[i];
		cfa->setTempVVal(args[i]);
		Type *cft = cf->getArg(i);
		cfa->setType(cft, true);
		tmgr.addVar(cfa->getName().getDataStr(), cft, cfa);
	}
	cfsig->getRetType()->setType(cf->getRet());
	cfsig->setType(cf);
	cfdef->setType(cf);
	cfvar->setType(cf);
	tmgr.pushFunc(cf);
	if(!visit(cfblk, asStmt(&cfblk))) {
		err.set(caller, "failed to assign type for called template function's var");
		return false;
	}
	tmgr.popFunc();
	tmgr.popLayer();

	if(caller->getMod()->getID() == cfdef->getMod()->getID()) {
		tmgr.unlockScope();
	}

	cfvarptr.unset();
	specfns.push_back(cfvar);
	return true;
}
} // namespace sc