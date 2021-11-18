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
	: Pass(Pass::genPassID<TypeAssignPass>(), err, ctx), vmgr(ctx), vpass(err, ctx), valen(0),
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
	if(stmt && stmt->getValueID() && stmt->getValueTy()->hasComptime() &&
	   !stmt->getValueTy()->isTemplate())
	{
		if(!vpass.visit(stmt, source)) {
			err.set(stmt, "failed to get value for a comptime type");
			return false;
		}
	}
	return res;
}

bool TypeAssignPass::visit(StmtBlock *stmt, Stmt **source)
{
	if(stmt->getMod()->isMainModule() || !stmt->isTop()) vmgr.pushLayer();

	if(stmt->isTop()) deferstack.pushFunc();

	auto &stmts = stmt->getStmts();
	deferstack.pushFrame();
	bool inserted_defers = false;
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(stmts[i]->isReturn() && !inserted_defers) {
			std::vector<Stmt *> deferred = deferstack.getAllStmts();
			stmts.insert(stmts.begin() + i, deferred.begin(), deferred.end());
			inserted_defers = true;
			--i;
			continue;
		}
		if(!visit(stmts[i], &stmts[i])) {
			err.set(stmt, "failed to assign type to stmt in block");
			return false;
		}
		if(!stmts[i]) {
			stmts.erase(stmts.begin() + i);
			--i;
		}
		if(i != stmts.size() - 1 || inserted_defers) continue;
		std::vector<Stmt *> &deferred = deferstack.getTopStmts();
		stmts.insert(stmts.end(), deferred.begin(), deferred.end());
		inserted_defers = true;
	}
	deferstack.popFrame();

	// insert all the specfns
	if(stmt->isTop()) {
		stmts.insert(stmts.begin(), specfns.begin(), specfns.end());
		specfns.clear();
	}

	if(stmt->isTop()) deferstack.popFunc();

	if(stmt->getMod()->isMainModule() || !stmt->isTop()) vmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtType *stmt, Stmt **source)
{
	if(!visit(stmt->getExpr(), &stmt->getExpr()) || !stmt->getExpr()->getValue()) {
		err.set(stmt, "failed to determine type of type-expr");
		return false;
	}
	Type *res = stmt->getExpr()->getValueTy()->clone(ctx);
	res->setInfo(stmt->getInfoMask());
	// generate ptrs
	for(size_t i = 0; i < stmt->getPtrCount(); ++i) {
		res = PtrTy::create(ctx, res, 0);
	}
	stmt->createAndSetValue(TypeVal::create(ctx, res));
	return true;
}
bool TypeAssignPass::visit(StmtSimple *stmt, Stmt **source)
{
	lex::Lexeme &lval = stmt->getLexValue();
	switch(lval.getTok().getVal()) {
	case lex::VOID: stmt->createAndSetValue(VoidVal::create(ctx)); break;
	case lex::ANY: stmt->createAndSetValue(TypeVal::create(ctx, AnyTy::create(ctx))); break;
	case lex::TYPE: stmt->createAndSetValue(TypeVal::create(ctx, TypeTy::create(ctx))); break;
	case lex::TRUE: {
		IntVal *iv = IntVal::create(ctx, mkI1Ty(ctx), CDPERMA, 1);
		stmt->createAndSetValue(iv);
		break;
	}
	case lex::FALSE: // fallthrough
	case lex::NIL: stmt->createAndSetValue(IntVal::create(ctx, mkI1Ty(ctx), CDPERMA, 0)); break;
	case lex::CHAR: {
		IntVal *iv = IntVal::create(ctx, mkI8Ty(ctx), CDPERMA, lval.getDataInt());
		stmt->createAndSetValue(iv);
		break;
	}
	case lex::INT: {
		IntVal *iv = IntVal::create(ctx, mkI32Ty(ctx), CDPERMA, lval.getDataInt());
		stmt->createAndSetValue(iv);
		break;
	}
	case lex::FLT: {
		FltVal *fv = FltVal::create(ctx, mkF32Ty(ctx), CDPERMA, lval.getDataFlt());
		stmt->createAndSetValue(fv);
		break;
	}
	case lex::STR: stmt->createAndSetValue(VecVal::createStr(ctx, lval.getDataStr())); break;
	case lex::I1: stmt->createAndSetValue(TypeVal::create(ctx, mkI1Ty(ctx))); break;
	case lex::I8: stmt->createAndSetValue(TypeVal::create(ctx, mkI8Ty(ctx))); break;
	case lex::I16: stmt->createAndSetValue(TypeVal::create(ctx, mkI16Ty(ctx))); break;
	case lex::I32: stmt->createAndSetValue(TypeVal::create(ctx, mkI32Ty(ctx))); break;
	case lex::I64: stmt->createAndSetValue(TypeVal::create(ctx, mkI64Ty(ctx))); break;
	case lex::U8: stmt->createAndSetValue(TypeVal::create(ctx, mkU8Ty(ctx))); break;
	case lex::U16: stmt->createAndSetValue(TypeVal::create(ctx, mkU16Ty(ctx))); break;
	case lex::U32: stmt->createAndSetValue(TypeVal::create(ctx, mkU32Ty(ctx))); break;
	case lex::U64: stmt->createAndSetValue(TypeVal::create(ctx, mkU64Ty(ctx))); break;
	case lex::F32: stmt->createAndSetValue(TypeVal::create(ctx, mkF32Ty(ctx))); break;
	case lex::F64: stmt->createAndSetValue(TypeVal::create(ctx, mkF64Ty(ctx))); break;
	case lex::IDEN: {
		StmtVar *decl		 = nullptr;
		Module *mod		 = stmt->getMod();
		std::string name	 = stmt->getLexValue().getDataStr();
		std::string mangled_name = getMangledName(stmt, name);
		if(!stmt->isAppliedModuleID()) {
			stmt->setValueID(vmgr.getVar(mangled_name, false, true));
			decl = vmgr.getDecl(mangled_name, false, true);
			if(stmt->getValueID()) stmt->updateLexDataStr(mangled_name);
		}
		if(!stmt->getValueID()) {
			stmt->setValueID(vmgr.getVar(name, false, true));
			decl = vmgr.getDecl(name, false, true);
		}
		stmt->setAppliedModuleID(true);
		stmt->setDecl(decl);
		break;
	}
	default: return false;
	}
	if(!stmt->getValueID()) {
		err.set(stmt, "undefined variable: %s", stmt->getLexValue().getDataStr().c_str());
		return false;
	}
	return true;
}
bool TypeAssignPass::visit(StmtFnCallInfo *stmt, Stmt **source)
{
	vmgr.pushLayer();
	for(auto &a : stmt->getArgs()) {
		if(!visit(a, asStmt(&a))) {
			err.set(stmt, "failed to determine type of argument");
			return false;
		}
	}
	vmgr.popLayer();
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
		if(!lhs->getValueTy()->isPtr()) {
			err.set(lhs, "LHS must be a pointer for arrow access");
			return false;
		}
	}
	case lex::DOT: {
		assert(rhs && rhs->isSimple() && "RHS stmt type for dot operation must be simple");
		StmtSimple *rsim = as<StmtSimple>(rhs);
		if(lhs->getValue()->isImport()) {
			const std::string &rdata = rsim->getLexValue().getDataStr();
			ImportVal *import	 = as<ImportVal>(lhs->getValue());
			std::string mangled	 = getMangledName(rhs, rdata, import);
			rsim->updateLexDataStr(mangled);
			rsim->setAppliedModuleID(true);
			if(!visit(stmt->getRHS(), &stmt->getRHS())) {
				err.set(stmt, "failed to determine type of RHS in dot expression");
				return false;
			}
			// replace this stmt with RHS (effectively removing LHS - import)
			*source = rhs;
			// once the source is changed, stmt will also be invalidated - therefore,
			// cannot be used anymore
			return true;
		}
		const std::string &fieldname = rsim->getLexValue().getDataStr();
		// if value is struct, that's definitely not struct def
		// struct def will be in TypeVal(Struct)
		if(!lhs->getValue()->isStruct()) {
			err.set(stmt,
				"dot operation can be performed "
				"only on imports and structs, found: %s",
				lhs->getValue()->toStr().c_str());
			return false;
		}
		StructVal *sv = as<StructVal>(lhs->getValue());
		// struct field names are not mangled with the module ids
		Value *res = sv->getField(fieldname);
		if(res) {
			rhs->createAndSetValue(res);
			stmt->setValueID(rhs);
			break;
		}
		uint64_t vid = 0;
		if(!(vid = vmgr.getTypeFn(lhs->getValueTy(), fieldname))) {
			err.set(stmt, "no field or function '%s' in struct '%s'", fieldname.c_str(),
				lhs->getValue()->toStr().c_str());
			return false;
		}
		// erase the expression, replace with the rhs alone
		rsim->setSelf(lhs);
		rsim->createAndSetValue(getValueWithID(vid));
		*source = rsim;
		// can't do anything after source modification since stmt not of same type
		return true;
	}
	case lex::FNCALL: {
		assert(lhs->isSimple() && "LHS must be a simple expression for function call");
		assert(rhs && rhs->isFnCallInfo() &&
		       "RHS must be function call info for a function call");
		// not using getValue() here as a struct def is not contained in it
		// a struct def = getValue()->isType() && getValueTy()->isStruct()
		if(!lhs->getValueTy()->isFunc() &&
		   !(lhs->getValue()->isType() && lhs->getValueTy()->isStruct())) {
			err.set(stmt, "func call can be performed only on funcs or struct defs");
			return false;
		}
		StmtFnCallInfo *callinfo  = as<StmtFnCallInfo>(rhs);
		std::vector<Stmt *> &args = callinfo->getArgs();
		if(lhs->isSimple() && as<StmtSimple>(lhs)->getSelf()) {
			args.insert(args.begin(), as<StmtSimple>(lhs)->getSelf());
		}
		if(lhs->getValue()->isFunc()) {
			TypeVal *fnval = as<TypeVal>(lhs->getValue());
			FuncTy *fn     = as<FuncTy>(fnval->getVal());
			bool has_va    = fn->hasVariadic();
			if(!(fn = fn->createCall(ctx, err, stmt->getLoc(), args))) {
				err.set(stmt, "function is incompatible with call arguments");
				return false;
			}
			fnval = TypeVal::create(ctx, fn);
			lhs->createAndSetValue(fnval);
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
			// clone() is called to resolve any TypeTy's
			Value *retval = fn->getRet()->toDefaultValue(ctx, err, stmt->getLoc());
			if(!retval) {
				err.set(stmt,
					"failed to generate a default"
					" value for function return type: %s",
					fn->getRet()->toStr().c_str());
				return false;
			}
			stmt->createAndSetValue(retval);
			if(stmt->isIntrinsicCall()) {
				if(!fn->isIntrinsic()) {
					err.set(stmt, "function call is intrinsic but the function "
						      "itself is not");
					return false;
				}
				if(fn->isParseIntrinsic() &&
				   !fn->callIntrinsic(ctx, err, stmt, source, args)) {
					err.set(stmt, "call to parse intrinsic failed");
					return false;
				}
				stmt->setCalledFnTy(fn);
				break;
			} else if(fn->isIntrinsic()) {
				err.set(stmt, "function is intrinsic - required '@' before call");
				return false;
			}
			stmt->setCalledFnTy(fn);
			// apply template specialization
			if(!initTemplateFunc(stmt, fn, args)) return false;
		} else if(lhs->getValueTy()->isStruct()) {
			StructTy *st = as<StructTy>(lhs->getValueTy());
			std::vector<Type *> argtypes;
			for(auto &a : args) {
				argtypes.push_back(a->getValueTy());
			}
			StructTy *resst = st->applyTemplates(ctx, err, stmt->getLoc(), argtypes);
			if(!resst) {
				err.set(stmt, "failed to specialize struct");
				return false;
			}
			lhs->createAndSetValue(TypeVal::create(ctx, resst));
			*source = lhs;
			return true;
		}
		break;
	}
	case lex::STCALL: {
		Value *&lv = lhs->getValue();
		if(!lv->isType() || !as<TypeVal>(lv)->getVal()->isStruct()) {
			err.set(stmt,
				"struct call is only applicable on struct definitions, found: %s",
				lv->toStr().c_str());
			return false;
		}
		StructTy *st		  = as<StructTy>(lhs->getValueTy());
		StmtFnCallInfo *callinfo  = as<StmtFnCallInfo>(rhs);
		std::vector<Stmt *> &args = callinfo->getArgs();
		if(!(st = st->instantiate(ctx, err, stmt->getLoc(), callinfo->getArgs()))) {
			err.set(stmt, "failed to instantiate struct with given arguments");
			return false;
		}
		size_t fnarglen	  = st->getFields().size();
		size_t callarglen = callinfo->getArgs().size();
		std::unordered_map<std::string, Value *> stvals;
		for(size_t i = 0; i < fnarglen; ++i) {
			Type *coerced_to = st->getField(i);
			applyPrimitiveTypeCoercion(coerced_to, callinfo->getArg(i));
			stvals[st->getFieldName(i)] = callinfo->getArg(i)->getValue();
		}
		StructVal *sv = StructVal::create(ctx, st, CDFALSE, stvals);
		stmt->createAndSetValue(sv);
		break;
	}
	// address of
	case lex::UAND: {
		if(!lhs->getValue()->isType()) {
			Type *t = lhs->getValueTy();
			t	= PtrTy::create(ctx, t, 0);
			stmt->createAndSetValue(RefVal::create(ctx, t, lhs->getValue()));
			break;
		}
		Type *t = as<TypeVal>(lhs->getValue())->getVal();
		t	= t->clone(ctx);
		t->setRef();
		stmt->createAndSetValue(TypeVal::create(ctx, t));
		break;
	}
	// dereference
	case lex::UMUL: {
		if(!lhs->getValue()->isType()) {
			Type *t = lhs->getValueTy();
			if(!t->isPtr()) {
				err.set(stmt, "cannot dereference non pointer type: %s",
					t->toStr().c_str());
				return false;
			}
			t = as<PtrTy>(t)->getTo();
			stmt->createAndSetValue(RefVal::create(ctx, t, lhs->getValue()));
			break;
		}
		Type *t = as<TypeVal>(lhs->getValue())->getVal();
		stmt->createAndSetValue(TypeVal::create(ctx, PtrTy::create(ctx, t, 0)));
		break;
	}
	case lex::SUBS: {
		if(lhs->getValueTy()->isVariadic()) {
			if(!lhs->isSimple()) {
				err.set(stmt, "LHS in variadic subscript must be a simple stmt");
				return false;
			}
			if(!rhs->getValue()->isInt()) {
				err.set(rhs, "index for a variadic must be integral");
				return false;
			}
			if(!rhs->getValue()->hasData() && !vpass.visit(rhs, &rhs)) {
				err.set(stmt, "variadic index must be calculable at comptime");
				return false;
			}
			IntVal *iv = as<IntVal>(rhs->getValue());
			if(getFnVALen() <= iv->getVal()) {
				err.set(stmt,
					"variadic index out of bounds "
					"(va: %zu, index: %" PRId64 ")",
					getFnVALen(), iv->getVal());
				return false;
			}
			StmtSimple *l	 = as<StmtSimple>(lhs->clone(ctx));
			std::string newn = l->getLexValue().getDataStr();
			newn += "." + std::to_string(iv->getVal());
			l->getLexValue().setDataStr(newn);
			*source = l;
			if(!visit(*source, source)) {
				err.set(stmt, "failed to determine type of LHS in expression");
				return false;
			}
			return true;
		} else if(lhs->getValueTy()->isPtr()) {
			if(!rhs->getValue()->isInt()) {
				err.set(rhs, "index for a pointer must be integral");
				return false;
			}
			Type *t = as<PtrTy>(lhs->getValueTy())->getTo();
			stmt->createAndSetValue(t->toDefaultValue(ctx, err, stmt->getLoc()));
			break;
		}
		goto applyoperfn;
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
		uint64_t fnid	= vmgr.getTypeFn(lhs->getValueTy(), optok.getOperCStr());
		FuncVal *fnval	= as<FuncVal>(getValueWithID(fnid));
		if(!fnval) {
			err.set(stmt, "function '%s' does not exist for type: %s",
				optok.getOperCStr(), lhs->getValueTy()->toStr().c_str());
			return false;
		}
		FuncTy *fn = as<FuncTy>(fnval->getType());

		std::vector<Stmt *> args = {lhs};
		if(rhs) args.push_back(rhs);
		if(!(fn = fn->createCall(ctx, err, stmt->getLoc(), args))) {
			err.set(stmt, "function is incompatible with call arguments");
			return false;
		}
		Value *retval = fn->getRet()->toDefaultValue(ctx, err, stmt->getLoc());
		if(!retval) {
			err.set(stmt,
				"failed to generate a default"
				" value for function return type: %s",
				fn->getRet()->toStr().c_str());
			return false;
		}
		stmt->createAndSetValue(retval);
		bool both_comptime = true;
		if(!lhs->getValueTy()->hasComptime()) both_comptime = false;
		if(rhs && !rhs->getValueTy()->hasComptime()) both_comptime = false;
		if(stmt->getValueTy()->hasComptime() && !both_comptime) {
			stmt->getValueTy()->unsetComptime();
		}

		for(size_t i = 0; i < args.size(); ++i) {
			Type *coerced_to = fn->getArg(i);
			Stmt *&arg	 = args[i];
			if(!coerced_to->hasComptime() || vpass.visit(args[i], &args[i])) {
				continue;
			}
			err.set(stmt, "failed to determine value for comptime arg");
			return false;
		}

		if(fn->isParseIntrinsic() && both_comptime &&
		   !fn->callIntrinsic(ctx, err, stmt, source, args)) {
			err.set(stmt, "call to parse intrinsic failed");
			return false;
		}
		stmt->setCalledFnTy(fn);
		if(!initTemplateFunc(stmt, fn, args)) {
			err.set(stmt, "failed to intialize template function");
			return false;
		}
		break;
	}
	default: err.set(stmt->getOper(), "nonexistent operator"); return false;
	}
	if(stmt->getCommas() > 0) {
		stmt->setValueID((uint64_t)0);
	}
	return true;
}
bool TypeAssignPass::visit(StmtVar *stmt, Stmt **source)
{
	Stmt *&val	 = stmt->getVVal();
	StmtType *&vtype = stmt->getVType();
	Type *inty	 = nullptr;
	if(val && val->isFnDef()) {
		as<StmtFnDef>(val)->setParentVar(stmt);
		if(stmt->isIn()) goto post_mangling;
	}
	if(stmt->isGlobal()) goto post_mangling;
	if(disabled_varname_mangling || stmt->isAppliedModuleID()) goto post_mangling;
	stmt->getName().setDataStr(getMangledName(stmt, stmt->getName().getDataStr()));
	stmt->setAppliedModuleID(true);
post_mangling:
	if(val && (!visit(val, &val) || !val->getValueTy())) {
		err.set(stmt, "unable to determine type of value of this variable");
		return false;
	}
	if(vtype && (!visit(vtype, asStmt(&vtype)) || !vtype->getValueTy())) {
		err.set(stmt, "unable to determine type from the given type of this variable");
		return false;
	}
	if(stmt->isIn()) {
		StmtFnDef *def = as<StmtFnDef>(stmt->getVVal());
		StmtVar *self  = def->getSigArgs()[0];
		if(vmgr.existsTypeFn(self->getValueTy(), stmt->getName().getDataStr())) {
			err.set(stmt, "member function '%s' already exists for type: %s",
				stmt->getName().getDataStr().c_str(),
				self->getValueTy()->toStr().c_str());
			return false;
		}
	}
	if(!stmt->isIn() && vmgr.exists(stmt->getName().getDataStr(), true, false)) {
		err.set(stmt->getName(), "variable '%s' already exists in scope",
			stmt->getName().getDataStr().c_str());
		return false;
	}
	if(val && val->getValueTy()->isVoid()) {
		err.set(stmt, "value expression returns void, which cannot be assigned to a var");
		return false;
	}
	if(vtype && val &&
	   !vtype->getValueTy()->isCompatible(ctx, val->getValueTy(), err, stmt->getLoc())) {
		err.set(stmt, "incompatible given type and value of the variable decl");
		return false;
	}
	if(val && stmt->isComptime()) {
		if(!vpass.visit(val, &val) || !val->getValue()->hasData()) {
			err.set(stmt, "value of comptime variable could not be calculated");
			return false;
		}
		val->getValueTy()->setComptime();
	}
	if(val && !vtype) {
		stmt->setValueID(val);
	} else if(vtype) {
		stmt->setValueID(vtype);
	}
	if(vtype && stmt->getValue()->isType()) {
		Type *t	   = as<TypeVal>(stmt->getValue())->getVal();
		Value *res = t->toDefaultValue(ctx, err, stmt->getLoc());
		if(!res) {
			err.set(stmt, "failed to retrieve default value for type: %s",
				t->toStr().c_str());
			return false;
		}
		// if(t->hasRef()) {
		// 	res = RefVal::create(ctx, t, res);
		// }
		stmt->createAndSetValue(res);
	}

	if(!stmt->getValueTy()->hasRef()) {
		stmt->createAndSetValue(stmt->getValue()->clone(ctx));
	}

	if(vtype && val) applyPrimitiveTypeCoercion(vtype->getValueTy(), val);
	if(stmt->isIn()) {
		StmtFnDef *def = as<StmtFnDef>(stmt->getVVal());
		StmtVar *self  = def->getSigArgs()[0];
		return vmgr.addTypeFn(self->getValueTy(), stmt->getName().getDataStr(),
				      stmt->getValueID());
	}
	return vmgr.addVar(stmt->getName().getDataStr(), stmt->getValueID(), stmt,
			   stmt->isGlobal());
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
		argst.push_back(a->getValueTy());
	}
	stmt->setScope(vmgr.getCurrentLayerIndex() - 1);
	Type *retty = stmt->getRetType()->getValueTy();
	FuncTy *ft  = FuncTy::create(ctx, nullptr, argst, retty, nullptr, INONE, false);
	stmt->createAndSetValue(FuncVal::create(ctx, ft));
	return true;
}
bool TypeAssignPass::visit(StmtFnDef *stmt, Stmt **source)
{
	vmgr.pushLayer();
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err.set(stmt, "failed to determine type of func signature");
		return false;
	}
	FuncVal *fn   = as<FuncVal>(stmt->getSig()->getValue());
	FuncTy *sigty = fn->getVal();

	sigty->setVar(stmt->getParentVar());

	if(stmt->requiresTemplateInit()) goto end;

	pushFunc(fn, false, 0);
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err.set(stmt, "failed to determine type of function block");
		return false;
	}
	popFunc();
end:
	stmt->setValueID(stmt->getSig());
	vmgr.popLayer();
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
	vmgr.pushLayer();
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err.set(stmt, "failed to determine type of func signature");
		return false;
	}
	FuncVal *fn = as<FuncVal>(stmt->getSig()->getValue());
	fn->getVal()->setExterned(true);
	if(stmt->getHeaders() && !visit(stmt->getHeaders(), asStmt(&stmt->getHeaders()))) {
		err.set(stmt, "failed to assign header type");
		return false;
	}
	if(stmt->getLibs() && !visit(stmt->getLibs(), asStmt(&stmt->getLibs()))) {
		err.set(stmt, "failed to assign lib type");
		return false;
	}
	stmt->setValueID(stmt->getSig());
	vmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtEnum *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtStruct *stmt, Stmt **source)
{
	vmgr.pushLayer();
	std::vector<std::string> fieldnames;
	std::vector<Type *> fieldtypes;
	std::vector<TypeTy *> templates;
	std::vector<std::string> templatenames = stmt->getTemplateNames();

	disabled_varname_mangling = true;
	for(auto &t : templatenames) {
		templates.push_back(TypeTy::create(ctx));
		uint64_t id = createValueIDWith(TypeVal::create(ctx, templates.back()));
		vmgr.addVar(t, id, nullptr);
	}
	for(auto &f : stmt->getFields()) {
		if(!visit(f, asStmt(&f))) {
			err.set(stmt, "failed to determine type of struct field");
			return false;
		}
		fieldnames.push_back(f->getName().getDataStr());
		fieldtypes.push_back(f->getValueTy());
	}
	disabled_varname_mangling = false;

	StructTy *st = StructTy::create(ctx, fieldnames, fieldtypes, templatenames, templates);
	stmt->createAndSetValue(TypeVal::create(ctx, st));
	vmgr.popLayer();
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
		if(!c->getValueTy()->isPrimitive()) {
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
		if(!c->getValue()->hasData()) {
			err.set(stmt, "inline condition received no value");
			return false;
		}
		if(c->getValue()->isInt()) {
			this_is_it = as<IntVal>(c->getValue())->getVal() != 0;
		}
		if(c->getValue()->isFlt()) {
			this_is_it = as<FltVal>(c->getValue())->getVal() != 0.0;
		}
		if(!this_is_it) continue;
	after_cond_check:
		if(!b) continue;
		if(!visit(b, asStmt(&b))) {
			err.set(stmt, "failed to determine types in inline conditional block");
			return false;
		}
		*source = b;
		(*source)->clearValue();
		return true;
	}
	// reached here && conditional is inline = delete the entire conditional
	if(stmt->isInline()) {
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

	vmgr.pushLayer();
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
	if(!cond->getValueTy()->isPrimitive()) {
		err.set(stmt, "inline for-loop's condition must be a primitive (int/flt)");
		return false;
	}

	if(!stmt->isInline() && blk && !visit(blk, asStmt(&blk))) {
		err.set(stmt, "failed to determine type of for-loop block");
		return false;
	}
	if(!stmt->isInline()) {
		vmgr.popLayer();
		return true;
	}

	if(!blk) {
		*source = nullptr;
		return true;
	}

	if(init && !vpass.visit(init, &init)) {
		err.set(stmt, "failed to determine value of inline for-loop init expr");
		return false;
	}
	if(!vpass.visit(cond, &cond)) {
		err.set(stmt, "failed to determine value of inline for-loop condition;"
			      " ensure relevant variables are comptime");
		return false;
	}
	if(init) newblkstmts.push_back(init->clone(ctx));
	while((cond->getValue()->isInt() && as<IntVal>(cond->getValue())->getVal()) ||
	      (cond->getValue()->isFlt() && as<FltVal>(cond->getValue())->getVal()))
	{
		for(auto &s : blk->getStmts()) {
			newblkstmts.push_back(s->clone(ctx));
		}
		if(incr) newblkstmts.push_back(incr->clone(ctx));
		if(incr && !vpass.visit(incr, &incr)) {
			err.set(stmt, "failed to determine value of inline for-loop incr");
			return false;
		}
		if(!vpass.visit(cond, &cond)) {
			err.set(stmt, "failed to determine value of inline for-loop condition");
			return false;
		}
	}
	blk->getStmts() = newblkstmts;
	finalblk	= blk;
	stmt->getBlk()	= nullptr;
	*source		= finalblk;
	vmgr.popLayer();
	if(!visit(*source, source)) {
		err.set(*source, "failed to determine type of inlined for-loop block");
		return false;
	}
	(*source)->clearValue();
	return true;
}
bool TypeAssignPass::visit(StmtWhile *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtRet *stmt, Stmt **source)
{
	if(!vmgr.hasFunc()) {
		err.set(stmt, "return statements can be in functions only");
		return false;
	}
	Stmt *&val = stmt->getVal();
	if(val && !visit(val, &val)) {
		err.set(stmt, "failed to determine type of the return argument");
		return false;
	}
	Type *valtype = val ? val->getValueTy() : VoidTy::create(ctx);
	FuncTy *fn    = vmgr.getTopFunc();
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
	stmt->setFnBlk(as<StmtFnDef>(fn->getVar()->getVVal())->getBlk());
	if(val) {
		stmt->setValueID(val);
	} else {
		stmt->createAndSetValue(VoidVal::create(ctx));
	}
	stmt->getFnBlk()->setValueID(stmt);
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
	deferstack.addStmt(stmt->getVal());
	*source = nullptr;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// Extra ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

std::string TypeAssignPass::getMangledName(Stmt *stmt, const std::string &name,
					   ImportVal *import) const
{
	if(stmt->isSimple()) {
		StmtSimple *sim = as<StmtSimple>(stmt);
		if(sim->isAppliedModuleID()) return name;
	}
	return name + "_" + (import ? import->getVal() : stmt->getMod()->getID());
}

void TypeAssignPass::applyPrimitiveTypeCoercion(Type *to, Stmt *from)
{
	if(!to || !from) return;
	if(!to->isPrimitiveOrPtr() || !from->getValueTy()->isPrimitiveOrPtr()) return;

	if(to->getID() == from->getValueTy()->getID()) return;
	from->castTo(to);
}

void TypeAssignPass::applyPrimitiveTypeCoercion(Stmt *lhs, Stmt *rhs, const lex::Lexeme &oper)
{
	if(!lhs || !rhs) return;
	if(!lhs->getValueTy()->isPrimitive() || !rhs->getValueTy()->isPrimitive()) return;
	if(oper.getTok().getVal() == lex::SUBS) return;

	Type *l = lhs->getValueTy();
	Type *r = rhs->getValueTy();

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
	cfvar = as<StmtVar>(cfvar->clone(ctx)); // template must be cloned
	cf->setVar(cfvar);
	cfdef		  = as<StmtFnDef>(cfvar->getVVal());
	StmtFnSig *&cfsig = cfdef->getSig();
	StmtBlock *&cfblk = cfdef->getBlk();
	cfsig->disableTemplates();
	cfsig->setVariadic(false);
	if(!cfblk) {
		err.set(caller, "function definition for specialization has no block");
		return false;
	}
	if(caller->getMod()->getID() == cfdef->getMod()->getID()) {
		vmgr.lockScopeBelow(cfsig->getScope());
	}

	vmgr.pushLayer();

	size_t va_count = 0;
	for(size_t i = 0; i < args.size(); ++i) {
		StmtVar *cfa = cfsig->getArg(i);
		Type *cft    = cf->getArg(i);
		if(!cft->isVariadic()) {
			if(cft->hasRef()) {
				cfa->setValueID(args[i]);
			} else {
				cfa->createAndSetValue(args[i]->getValue()->clone(ctx));
			}
			vmgr.addVar(cfa->getName().getDataStr(), cfa->getValueID(), cfa);
			continue;
		}
		ModuleLoc &mloc	     = cfa->getLoc();
		lex::Lexeme &va_name = cfa->getName();
		Type *vaty	     = cft;
		cfsig->getArgs().pop_back();
		cf->getArgs().pop_back();
		std::vector<Value *> vtmp(args.size() - i, nullptr);
		uint64_t vavid = createValueIDWith(VecVal::create(ctx, vaty, CDFALSE, vtmp));
		vmgr.addVar(va_name.getDataStr(), vavid, cfa);
		while(i < args.size()) {
			std::string argn = va_name.getDataStr() + "." + std::to_string(va_count);
			StmtVar *newv	 = as<StmtVar>(cfa->clone(ctx));
			newv->getName().setDataStr(argn);
			Type *t = args[i]->getValueTy()->clone(ctx);
			t->appendInfo(cft->getInfo());
			if(t->hasRef()) {
				newv->setValueID(args[i]);
			} else {
				newv->createAndSetValue(args[i]->getValue()->clone(ctx));
			}
			vmgr.addVar(argn, newv->getValueID(), newv);
			cfsig->getArgs().push_back(newv);
			cf->getArgs().push_back(t);
			++va_count;
			++i;
		}
		break;
	}
	FuncVal *cfn = FuncVal::create(ctx, cf);
	cfsig->getRetType()->createAndSetValue(TypeVal::create(ctx, cf->getRet()));
	cfsig->createAndSetValue(cfn);
	cfdef->setValueID(cfsig);
	cfvar->setValueID(cfsig);
	pushFunc(cfn, va_count > 0, va_count);
	if(!visit(cfblk, asStmt(&cfblk))) {
		err.set(caller, "failed to assign type for called template function's var");
		return false;
	}
	popFunc();
	vmgr.popLayer();

	if(caller->getMod()->getID() == cfdef->getMod()->getID()) {
		vmgr.unlockScope();
	}

	specfns.push_back(cfvar);
	return true;
}

void TypeAssignPass::pushFunc(FuncVal *fn, const bool &is_va, const size_t &va_len)
{
	vmgr.pushFunc(fn->getVal());
	deferstack.pushFunc();
	is_fn_va.push_back(is_va);
	valen.push_back(va_len);
}
void TypeAssignPass::popFunc()
{
	deferstack.popFunc();
	vmgr.popFunc();
	is_fn_va.pop_back();
	valen.pop_back();
}

} // namespace sc