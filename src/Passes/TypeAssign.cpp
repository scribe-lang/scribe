#include "Passes/TypeAssign.hpp"

#include <climits>
#include <float.h>

#include "Parser.hpp"
#include "Utils.hpp"

namespace sc
{
TypeAssignPass::TypeAssignPass(Context &ctx)
	: Pass(Pass::genPassID<TypeAssignPass>(), ctx), vmgr(ctx), vpass(ctx), valen(0),
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
	case FOR: res = visit(as<StmtFor>(stmt), source); break;
	case RET: res = visit(as<StmtRet>(stmt), source); break;
	case CONTINUE: res = visit(as<StmtContinue>(stmt), source); break;
	case BREAK: res = visit(as<StmtBreak>(stmt), source); break;
	case DEFER: res = visit(as<StmtDefer>(stmt), source); break;
	default: {
		err::out(stmt, {"invalid statement found for type assignment: ",
				stmt->getStmtTypeCString()});
		break;
	}
	}
	if(!res) return false;
	if(!source || !*source) return res;
	stmt = *source;
	if(stmt->getTy() && stmt->getTy()->isStruct() && as<StructTy>(stmt->getTy())->getDecl() &&
	   as<StructTy>(stmt->getTy())->getDecl()->isDecl())
	{
		deferredspecialize.pushDataInternal(stmt->getTy()->getID(), &stmt->getTy());
	}
	if(stmt->getVal() && stmt->getVal()->isType() &&
	   as<TypeVal>(stmt->getVal())->getVal()->isStruct() &&
	   as<StructTy>(as<TypeVal>(stmt->getVal())->getVal())->getDecl() &&
	   as<StructTy>(as<TypeVal>(stmt->getVal())->getVal())->getDecl()->isDecl())
	{
		Type *&t = as<TypeVal>(stmt->getVal())->getVal();
		deferredspecialize.pushDataInternal(t->getID(), &t);
	}
	if(stmt->isComptime() && !stmt->getTy()->isTemplate()) {
		if(!vpass.visit(stmt, source)) {
			err::out(stmt, {"failed to get value for a comptime type"});
			return false;
		}
	}
	return res;
}

bool TypeAssignPass::visit(StmtBlock *stmt, Stmt **source)
{
	if(!stmt->isLayeringDisabled()) {
		if(stmt->getMod()->isMainModule() || !stmt->isTop()) vmgr.pushLayer();
	}

	if(stmt->isTop()) deferstack.pushFunc();

	auto &stmts = stmt->getStmts();
	deferstack.pushFrame();
	bool inserted_defers = false;
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(stmts[i]->isReturn() && !inserted_defers) {
			Vector<Stmt *> deferred = deferstack.getAllStmts(ctx);
			stmts.insert(stmts.begin() + i, deferred.begin(), deferred.end());
			inserted_defers = true;
			--i;
			continue;
		}
		if(!visit(stmts[i], &stmts[i])) {
			err::out(stmt, {"failed to assign type to stmt in block"});
			return false;
		}
		if(!stmts[i]) {
			stmts.erase(stmts.begin() + i);
			--i;
			continue;
		}
		// remove blocks at top level (also helps with conditional imports)
		if(stmt->isTop() && stmts[i]->isBlock()) {
			StmtBlock *blk		 = as<StmtBlock>(stmts[i]);
			Vector<Stmt *> &blkstmts = blk->getStmts();
			stmts.erase(stmts.begin() + i);
			stmts.insert(stmts.begin() + i, blkstmts.begin(), blkstmts.end());
			i += blkstmts.size();
			--i;
		}
		if(i != stmts.size() - 1 || inserted_defers) continue;
		Vector<Stmt *> deferred = deferstack.getTopStmts(ctx);
		stmts.insert(stmts.end(), deferred.begin(), deferred.end());
		inserted_defers = true;
	}
	deferstack.popFrame();

	// insert all the additionalvars
	if(stmt->isTop()) {
		stmts.insert(stmts.begin(), additionalvars.begin(), additionalvars.end());
		additionalvars.clear();
	}

	if(stmt->isTop()) deferstack.popFunc();

	if(!stmt->isLayeringDisabled()) {
		if(stmt->getMod()->isMainModule() || !stmt->isTop()) vmgr.popLayer();
	}
	return true;
}
bool TypeAssignPass::visit(StmtType *stmt, Stmt **source)
{
	// TODO: add array type
	if(!visit(stmt->getExpr(), &stmt->getExpr()) ||
	   (!stmt->getExpr()->getVal() && !stmt->getExpr()->getTy()))
	{
		err::out(stmt, {"failed to determine type of type-expr"});
		return false;
	}
	bool is_self = false;
	// self referenced struct - must be a reference or pointer
	if(stmt->getExpr()->isSimple() &&
	   as<StmtSimple>(stmt->getExpr())->getLexValue().getDataStr() == "Self")
	{
		if(!stmt->getPtrCount()) {
			err::out(stmt, {"self referencing struct member must be a pointer"});
			return false;
		}
		is_self = true;
	}
	Type *res = stmt->getExpr()->getTy();
	// for cross-referencing structs
	bool is_struct_decl =
	res->isStruct() && as<StructTy>(res)->getDecl() && as<StructTy>(res)->getDecl()->isDecl();
	if(!is_self && !is_struct_decl) {
		res = res->specialize(ctx);
	}
	// generate ptrs
	for(size_t i = 0; i < stmt->getPtrCount(); ++i) {
		res = PtrTy::get(ctx, res, 0, false);
		if(i == 0 && is_struct_decl) {
			deferredspecialize.pushDataInternal(as<PtrTy>(res)->getTo()->getID(),
							    &as<PtrTy>(res)->getTo());
		}
		// self referencing or cross-referencing structs must be made as weak pointers
		if((is_self || is_struct_decl) && i == stmt->getPtrCount() - 1) {
			as<PtrTy>(res)->setWeak(true);
		}
	}
	stmt->setTyVal(res, TypeVal::create(ctx, res));
	return true;
}
bool TypeAssignPass::visit(StmtSimple *stmt, Stmt **source)
{
	lex::Lexeme &tok = stmt->getLexValue();
	switch(tok.getTokVal()) {
	case lex::VOID: stmt->setTypeVal(ctx, VoidTy::get(ctx)); break;
	case lex::ANY: stmt->setTypeVal(ctx, AnyTy::get(ctx)); break;
	case lex::TYPE: stmt->setTypeVal(ctx, TypeTy::get(ctx)); break;
	case lex::TRUE: {
		stmt->setTyVal(IntTy::get(ctx, 1, true), IntVal::create(ctx, CDPERMA, 1));
		break;
	}
	case lex::FALSE: // fallthrough
	case lex::NIL: {
		stmt->setTyVal(IntTy::get(ctx, 1, true), IntVal::create(ctx, CDPERMA, 0));
		break;
	}
	case lex::CHAR: {
		stmt->setTyVal(IntTy::get(ctx, 8, true),
			       IntVal::create(ctx, CDPERMA, tok.getDataStr().front()));
		break;
	}
	case lex::INT: {
		Type *ity = nullptr;
		if(tok.getDataInt() > INT_MAX || tok.getDataInt() < INT_MIN) {
			ity = IntTy::get(ctx, 64, true);
		} else {
			ity = IntTy::get(ctx, 32, true);
		}
		stmt->setTyVal(ity, IntVal::create(ctx, CDPERMA, tok.getDataInt()));
		break;
	}
	case lex::FLT: {
		Type *fty = nullptr;
		if(tok.getDataFlt() > FLT_MAX || tok.getDataFlt() < FLT_MIN) {
			fty = FltTy::get(ctx, 64);
		} else {
			fty = FltTy::get(ctx, 32);
		}
		stmt->setTyVal(fty, FltVal::create(ctx, CDPERMA, tok.getDataFlt()));
		break;
	}
	case lex::STR: {
		stmt->setTyVal(PtrTy::getStr(ctx),
			       VecVal::createStr(ctx, CDPERMA, tok.getDataStr()));
		stmt->setConst();
		break;
	}
	case lex::I1: stmt->setTypeVal(ctx, IntTy::get(ctx, 1, true)); break;
	case lex::I8: stmt->setTypeVal(ctx, IntTy::get(ctx, 8, true)); break;
	case lex::I16: stmt->setTypeVal(ctx, IntTy::get(ctx, 16, true)); break;
	case lex::I32: stmt->setTypeVal(ctx, IntTy::get(ctx, 32, true)); break;
	case lex::I64: stmt->setTypeVal(ctx, IntTy::get(ctx, 64, true)); break;
	case lex::U8: stmt->setTypeVal(ctx, IntTy::get(ctx, 8, false)); break;
	case lex::U16: stmt->setTypeVal(ctx, IntTy::get(ctx, 16, false)); break;
	case lex::U32: stmt->setTypeVal(ctx, IntTy::get(ctx, 32, false)); break;
	case lex::U64: stmt->setTypeVal(ctx, IntTy::get(ctx, 64, false)); break;
	case lex::F32: stmt->setTypeVal(ctx, FltTy::get(ctx, 32)); break;
	case lex::F64: stmt->setTypeVal(ctx, FltTy::get(ctx, 64)); break;
	case lex::IDEN: {
		StmtVar *decl  = nullptr;
		Module *mod    = stmt->getMod();
		StringRef name = stmt->getLexValue().getDataStr();
		VarDecl *res   = nullptr;
		if(!stmt->isAppliedModuleID()) {
			StringRef mangled_name = getMangledName(stmt, name);
			res		       = vmgr.getAll(mangled_name, false, true);
			if(res) stmt->updateLexDataStr(mangled_name);
		}
		if(!res) res = vmgr.getAll(name, false, true);
		if(!res) break; // error out - undefined variable
		stmt->setTyVal(res->ty, res->val);
		decl = res->decl;
		stmt->setAppliedModuleID(true);
		stmt->setDecl(decl);
		if(decl) stmt->setStmtMask(decl->getStmtMask());
		break;
	}
	default: return false;
	}
	if(!stmt->getTy()) {
		err::out(stmt, {"undefined variable: ", stmt->getLexValue().getDataStr()});
		return false;
	}
	return true;
}
bool TypeAssignPass::visit(StmtFnCallInfo *stmt, Stmt **source)
{
	vmgr.pushLayer();
	for(auto &a : stmt->getArgs()) {
		if(!visit(a, asStmt(&a))) {
			err::out(stmt, {"failed to determine type of argument"});
			return false;
		}
	}
	vmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtExpr *stmt, Stmt **source)
{
	if(stmt->getLHS() && !visit(stmt->getLHS(), &stmt->getLHS())) {
		err::out(stmt, {"failed to determine type of LHS in expression"});
		return false;
	}
	lex::TokType oper = stmt->getOper().getTokVal();
	if(oper != lex::DOT && oper != lex::ARROW && stmt->getRHS() &&
	   !visit(stmt->getRHS(), &stmt->getRHS()))
	{
		err::out(stmt, {"failed to determine type of RHS in expression"});
		return false;
	}
	Stmt *&lhs = stmt->getLHS();
	Stmt *&rhs = stmt->getRHS();
	switch(oper) {
	case lex::ARROW: {
		if(!lhs->getTy()->isPtr()) {
			err::out(lhs, {"LHS must be a pointer for arrow access"});
			return false;
		}
	}
	case lex::DOT: {
		assert(rhs && rhs->isSimple() && "RHS stmt type for dot operation must be simple");
		StmtSimple *rsim = as<StmtSimple>(rhs);
		if(lhs->getVal() && lhs->getVal()->isNamespace()) {
			StringRef rdata	     = rsim->getLexValue().getDataStr();
			NamespaceVal *import = as<NamespaceVal>(lhs->getVal());
			StringRef mangled    = getMangledName(rhs, rdata, import);
			rsim->updateLexDataStr(mangled);
			rsim->setAppliedModuleID(true);
			if(!visit(stmt->getRHS(), &stmt->getRHS())) {
				err::out(stmt,
					 {"failed to determine type of RHS in dot expression"});
				return false;
			}
			// replace this stmt with RHS (effectively removing LHS - import)
			*source = rhs;
			// once the source is changed, stmt will also be invalidated - therefore,
			// cannot be used anymore
			return true;
		}
		if(lhs->getVal() && lhs->getVal()->isType() && lhs->getTy()->isStruct()) {
			err::out(stmt, {"cannot use dot operator on a "
					"struct type - initialize it first"});
			return false;
		}
		StringRef fieldname = rsim->getLexValue().getDataStr();
		// if value is struct, that's definitely not struct def
		// struct def will be in TypeVal(Struct)
		StructTy *st	    = nullptr;
		Type *res	    = nullptr;
		Type *t		    = lhs->getTy();
		uint16_t derefcount = 0;
		while(t->isPtr()) {
			t = as<PtrTy>(t)->getTo();
			++derefcount;
		}
		lhs->setDerefCount(derefcount);
		if(!lhs->getTy()->isStruct()) {
			goto typefn;
		}
		st = as<StructTy>(lhs->getTy());
		// struct field names are not mangled with the module ids
		res = st->getField(fieldname);
		if(res) {
			rhs->setTy(res);
			if(st->isTemplateField(fieldname)) {
				rhs->setVal(TypeVal::create(ctx, res));
			}
			stmt->setTyVal(rhs);
			break;
		}
	typefn:
		FuncVal *fnv = nullptr;
		if(!(fnv = vmgr.getTyFn(lhs->getTy(), fieldname))) {
			err::out(stmt, {"no field or function '", fieldname, "' for type '",
					lhs->getTy()->toStr(), "'"});
			return false;
		}
		// erase the expression, replace with the rhs alone
		rsim->setSelf(lhs);
		rsim->setTyVal(fnv->getVal(), fnv);
		*source = rsim;
		// can't do anything after source modification since stmt not of same type
		return true;
	}
	case lex::FNCALL: {
		assert(lhs->isSimple() && "LHS must be a simple expression for function call");
		assert(rhs && rhs->isFnCallInfo() &&
		       "RHS must be function call info for a function call");
		// not using getVal() here as a struct def is not contained in it
		// a struct def = getVal()->isType() && getTy()->isStruct()
		if(!lhs->getVal() || !(lhs->getVal()->isFunc() ||
				       (lhs->getVal()->isType() && lhs->getTy()->isStruct())))
		{
			err::out(stmt, {"func call can be performed only on "
					"funcs or struct defs, found: ",
					lhs->getTy()->toStr()});
			return false;
		}
		StmtFnCallInfo *callinfo = as<StmtFnCallInfo>(rhs);
		Vector<Stmt *> &args	 = callinfo->getArgs();
		if(lhs->isSimple() && as<StmtSimple>(lhs)->getSelf()) {
			args.insert(args.begin(), as<StmtSimple>(lhs)->getSelf());
		}
		for(size_t i = 0; i < args.size(); ++i) {
			if(!args[i]->getTy()->isVariadic()) continue;
			StmtSimple *a = as<StmtSimple>(args[i]);
			if(a->getVal() && !a->getVal()->isVec()) {
				err::out(a, {"variadic value must be a vector"});
				return false;
			}
			args.erase(args.begin() + i);
			StringRef name = a->getLexValue().getDataStr();
			VariadicTy *vt = as<VariadicTy>(a->getTy());
			VecVal *vv     = nullptr;
			if(a->getVal()) vv = as<VecVal>(a->getVal());
			for(size_t j = 0; j < vt->getArgs().size(); ++j) {
				StringRef newn	 = ctx.strFrom({name, "__", ctx.strFrom(j)});
				StmtSimple *newa = as<StmtSimple>(a->clone(ctx));
				newa->getLexValue().setDataStr(newn);
				newa->setTy(vt->getArg(j));
				if(a->getVal()) newa->setVal(vv->getValAt(j));
				args.insert(args.begin() + i, newa);
				++i;
			}
			if(vt->getArgs().size() > 0) --i;
		}
		if(lhs->getVal()->isFunc()) {
			TypeVal *fnval = as<TypeVal>(lhs->getVal());
			FuncTy *fn     = as<FuncTy>(fnval->getVal());
			bool has_va    = fn->isVariadic();
			FuncTy *tmpfn  = fn;
			if(!(fn = fn->createCall(ctx, stmt->getLoc(), args))) {
				err::out(stmt, {"function '", tmpfn->toStr(),
						"' is incompatible with call arguments"});
				return false;
			}
			fnval = TypeVal::create(ctx, fn);
			lhs->setTyVal(fnval->getVal(), fnval);
			size_t fnarglen	  = fn->getArgs().size();
			size_t callarglen = args.size();
			for(size_t i = 0, j = 0, k = 0; i < fnarglen && j < callarglen; ++i, ++j) {
				Type *coerced_to = fn->getArg(i);
				Stmt *&arg	 = args[j];
				Stmt *fnarg	 = fn->getSig() ? fn->getSig()->getArg(i) : nullptr;
				bool fnargcomptime = fn->isArgComptime(i);
				bool fnargconst	   = fnarg ? fnarg->isConst() : arg->isConst();
				if(coerced_to->isVariadic()) {
					coerced_to = as<VariadicTy>(coerced_to)->getArg(k++);
					--i;
				}
				// type cast for const pointers
				if(fnarg && arg->isConst() != fnargconst && coerced_to->isPtr() &&
				   arg->getTy()->isPtr())
				{
					arg->castTo(coerced_to, fnarg->getStmtMask());
				}
				if(!arg->getCast()) applyPrimitiveTypeCoercion(coerced_to, arg);
				if(!fnargcomptime || (vpass.visit(arg, &arg) && arg->getVal())) {
					continue;
				}
				err::out(stmt, {"failed to determine value for comptime arg"});
				return false;
			}
			if(stmt->isIntrinsicCall()) {
				// clone() is called to resolve any TypeTy's
				stmt->setTy(fn->getRet());
				if(!fn->isIntrinsic()) {
					err::out(stmt, {"function call is intrinsic"
							"but the function itself is not"});
					return false;
				}
				if(fn->isParseIntrinsic() &&
				   !fn->callIntrinsic(ctx, stmt, source, args))
				{
					err::out(stmt, {"call to parse intrinsic failed"});
					return false;
				}
				stmt->setCalledFnTy(fn);
				break;
			} else if(fn->isIntrinsic()) {
				err::out(stmt,
					 {"function is intrinsic - required '@' before call"});
				return false;
			}
			// apply template specialization
			if(!initTemplateFunc(stmt, fn, args)) return false;
			fnval->setVal(fn);
			lhs->setTy(fn);
			stmt->setCalledFnTy(fn);
			stmt->setTy(fn->getRet());
			if(fn->getSig()) {
				stmt->appendStmtMask(fn->getSig()->getRetType()->getStmtMask());
			}
		} else if(lhs->getVal()->isType()) {
			StructTy *st = as<StructTy>(as<TypeVal>(lhs->getVal())->getVal());
			Vector<Type *> argtypes;
			for(auto &a : args) {
				argtypes.push_back(a->getTy());
			}
			// definition doesn't exist yet
			if(st->getDecl()->isDecl()) {
				lhs->setTypeVal(ctx, st);
				deferredspecialize.pushData(st->getID(), &lhs->getTy(), argtypes);
				deferredspecialize.pushData(
				st->getID(), &as<TypeVal>(lhs->getVal())->getVal(), argtypes);
			} else {
				StructTy *resst = st->applyTemplates(ctx, stmt->getLoc(), argtypes);
				if(!resst) {
					err::out(stmt, {"failed to specialize struct"});
					return false;
				}
				lhs->setTyVal(resst, TypeVal::create(ctx, resst));
			}
			*source = lhs;
			return true;
		}
		break;
	}
	case lex::STCALL: {
		Value *lv = lhs->getVal();
		if(!lv || !lv->isType() || !as<TypeVal>(lv)->getVal()->isStruct()) {
			err::out(stmt, {"struct call is only applicable"
					" on struct definitions, found: ",
					lhs->getTy()->toStr()});
			return false;
		}
		StructTy *st		 = as<StructTy>(as<TypeVal>(lv)->getVal());
		StmtFnCallInfo *callinfo = as<StmtFnCallInfo>(rhs);
		Vector<Stmt *> &args	 = callinfo->getArgs();
		if(!(st = st->instantiate(ctx, stmt->getLoc(), callinfo->getArgs()))) {
			err::out(stmt, {"failed to instantiate struct with given arguments"});
			return false;
		}
		size_t fnarglen	  = st->getFields().size();
		size_t callarglen = callinfo->getArgs().size();
		for(size_t i = 0; i < fnarglen; ++i) {
			Type *coerced_to = st->getField(i);
			applyPrimitiveTypeCoercion(coerced_to, callinfo->getArg(i));
		}
		stmt->setTy(st);
		break;
	}
	// address of
	case lex::UAND: {
		if(lhs->getVal() && lhs->getVal()->isType()) {
			err::out(stmt, {"cannot use address-of operator on a type"});
			return false;
		}
		stmt->setTy(PtrTy::get(ctx, lhs->getTy(), 0, false));
		break;
	}
	// dereference
	case lex::UMUL: {
		if(lhs->getVal() && lhs->getVal()->isType()) {
			Type *t = PtrTy::get(ctx, as<TypeVal>(lhs->getVal())->getVal(), 0, false);
			stmt->setTyVal(t, TypeVal::create(ctx, t));
			break;
		}
		Type *t = lhs->getTy();
		if(!t->isPtr()) {
			err::out(stmt, {"cannot dereference non pointer type: ", t->toStr()});
			return false;
		}
		stmt->setTy(as<PtrTy>(t)->getTo());
		break;
	}
	case lex::SUBS: {
		if(lhs->getTy()->isVariadic()) {
			if(!lhs->isSimple()) {
				err::out(stmt, {"LHS in variadic subscript must be a simple stmt"});
				return false;
			}
			if(!vpass.visit(rhs, &rhs) || !rhs->getVal()) {
				err::out(rhs, {"variadic index must be calculable at comptime"});
				return false;
			}
			if(!rhs->getVal()->isInt()) {
				err::out(rhs, {"index for a variadic must be integral"});
				return false;
			}
			IntVal *iv = as<IntVal>(rhs->getVal());
			if(getFnVALen() <= iv->getVal()) {
				err::out(stmt, {"variadic index out of bounds (va: ",
						ctx.strFrom(getFnVALen()),
						", index: ", ctx.strFrom(iv->getVal()), ")"});
				return false;
			}
			StmtSimple *l  = as<StmtSimple>(lhs->clone(ctx));
			StringRef newn = l->getLexValue().getDataStr();
			newn	       = ctx.strFrom({newn, "__", ctx.strFrom(iv->getVal())});
			l->getLexValue().setDataStr(newn);
			*source = l;
			if(!visit(*source, source)) {
				err::out(stmt, {"failed to determine type of LHS in expression"});
				return false;
			}
			return true;
		} else if(lhs->getTy()->isPtr()) {
			if(!rhs->getTy()->isInt()) {
				err::out(rhs, {"index for a pointer must be integral"});
				return false;
			}
			Type *t = as<PtrTy>(lhs->getTy())->getTo();
			stmt->setTy(t);
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
		FuncVal *fnval	= vmgr.getTyFn(lhs->getTy(), optok.getOperCStr());
		if(!fnval) {
			err::out(stmt, {"function '", optok.getOperCStr(),
					"' does not exist for type: ", lhs->getTy()->toStr()});
			return false;
		}
		if(optok.isAssign() && (lhs->isConst() || lhs->isCastConst()) &&
		   !lhs->getTy()->isPtr())
		{
			err::out(stmt, {"cannot perform assignment (like)"
					" operations on const data"});
			return false;
		}
		FuncTy *fn = as<FuncTy>(fnval->getVal());

		Vector<Stmt *> args = {lhs};
		if(rhs) args.push_back(rhs);
		for(size_t i = 0; i < args.size(); ++i) {
			if(!args[i]->getTy()->isVariadic()) continue;
			StmtSimple *a = as<StmtSimple>(args[i]);
			if(a->getVal() && !a->getVal()->isVec()) {
				err::out(a, {"variadic value must be a vector"});
				return false;
			}
			args.erase(args.begin() + i);
			StringRef name = a->getLexValue().getDataStr();
			VariadicTy *vt = as<VariadicTy>(a->getTy());
			VecVal *vv     = nullptr;
			if(a->getVal()) vv = as<VecVal>(a->getVal());
			for(size_t j = 0; j < vt->getArgs().size(); ++j) {
				StringRef newn	 = ctx.strFrom({name, "__", ctx.strFrom(j)});
				StmtSimple *newa = as<StmtSimple>(a->clone(ctx));
				newa->getLexValue().setDataStr(newn);
				newa->setTy(vt->getArg(j));
				if(a->getVal()) newa->setVal(vv->getValAt(j));
				args.insert(args.begin() + i, newa);
				++i;
			}
			if(vt->getArgs().size() > 0) --i;
		}
		if(!(fn = fn->createCall(ctx, stmt->getLoc(), args))) {
			stmt->disp(false);
			err::out(stmt, {"function is incompatible with call arguments"});
			return false;
		}
		bool both_comptime = true;
		if(!lhs->isComptime()) both_comptime = false;
		if(rhs && !rhs->isComptime()) both_comptime = false;

		// type cast for const pointers
		if(rhs) {
			Type *rhsty = rhs->getTy();
			bool fnrhsconst =
			fn->getSig() ? fn->getSig()->getArg(1)->isConst() : rhs->isConst();
			if(rhs && rhs->isConst() != fnrhsconst && fn->getArg(1)->isPtr() &&
			   rhsty->isPtr())
			{
				uint8_t rhsmask =
				fn->getSig() ? fn->getSig()->getArg(1)->getStmtMask() : 0;
				rhs->castTo(fn->getArg(1), rhsmask);
			}
		}

		for(size_t i = 0; i < args.size(); ++i) {
			Stmt *&arg	 = args[i];
			bool argcomptime = fn->isArgComptime(i);
			if((!both_comptime && !argcomptime) || vpass.visit(args[i], &args[i])) {
				continue;
			}
			err::out(stmt, {"failed to determine value for comptime arg"});
			return false;
		}

		if(fn->isParseIntrinsic()) {
			stmt->setTy(fn->getRet());
			if(!both_comptime) {
				err::out(stmt, {"arguments to parse intrinsic are not comptime"});
				return false;
			}
			if(!fn->callIntrinsic(ctx, stmt, source, args)) {
				err::out(stmt, {"call to parse intrinsic failed"});
				return false;
			}
			stmt->setCalledFnTy(fn);
			break;
		}
		if(!initTemplateFunc(stmt, fn, args)) {
			err::out(stmt, {"failed to intialize template function"});
			return false;
		}

		// don't return reference if both are primitive and operator ain't one of assignment
		bool both_primitive = true;
		if(!lhs->getTy()->isPrimitiveOrPtr()) both_primitive = false;
		if(rhs && !rhs->getTy()->isPrimitiveOrPtr()) both_primitive = false;
		if(both_primitive && !optok.isAssign() && fn->getSig()) {
			fn->getSig()->getRetType()->unsetRef();
		}

		// fnval->setVal(fn); is not required as fnval is used nowhere,
		// and it points to fnid which would therefore be modified elsewhere
		stmt->setCalledFnTy(fn);
		stmt->setTy(fn->getRet());
		if(fn->getSig()) {
			stmt->appendStmtMask(fn->getSig()->getRetType()->getStmtMask());
		}
		if(both_comptime) stmt->setComptime();
		break;
	}
	default: err::out(stmt->getOper(), {"nonexistent operator"}); return false;
	}
	if(!*source) return true;
	if(stmt->getCommas() > 0) {
		stmt->setTyVal(nullptr, nullptr);
	}
	return true;
}
bool TypeAssignPass::visit(StmtVar *stmt, Stmt **source)
{
	Stmt *&val	 = stmt->getVVal();
	StmtType *&vtype = stmt->getVType();
	Type *inty	 = nullptr;
	bool skip_val	 = false;
	if(val && val->isFnDef()) {
		as<StmtFnDef>(val)->setParentVar(stmt);
		if(stmt->isIn()) goto post_mangling;
	}
	if(val && val->isExtern()) {
		as<StmtExtern>(val)->setParentVar(stmt);
		if(!as<StmtExtern>(val)->getEntity()) skip_val = true;
	}
	if(stmt->isGlobal()) goto post_mangling;
	if(disabled_varname_mangling || stmt->isAppliedModuleID()) goto post_mangling;
	stmt->getName().setDataStr(getMangledName(stmt, stmt->getName().getDataStr()));
	stmt->setAppliedModuleID(true);
post_mangling:
	if(val && (!visit(val, &val) || (!skip_val && !val->getTy()))) {
		err::out(stmt, {"unable to determine type of value of this variable"});
		return false;
	}
	if(vtype && (!visit(vtype, asStmt(&vtype)) || !vtype->getTy())) {
		err::out(stmt, {"unable to determine type from the given type of this variable"});
		return false;
	}
	if(stmt->isIn()) {
		StmtFnDef *def = as<StmtFnDef>(stmt->getVVal());
		StmtVar *self  = def->getSigArgs()[0];
		if(vmgr.existsTypeFn(self->getTy(), stmt->getName().getDataStr())) {
			err::out(stmt, {"member function '", stmt->getName().getDataStr(),
					"' already exists for type: ", self->getTy()->toStr()});
			return false;
		}
	}
	if(!stmt->isIn() && vmgr.exists(stmt->getName().getDataStr(), true, stmt->isGlobal())) {
		VarDecl *d = vmgr.getAll(stmt->getName().getDataStr(), true, stmt->isGlobal());
		if(!d->val || !d->val->isType() || !d->ty->isStruct() ||
		   !as<StructTy>(d->ty)->getDecl()->isDecl())
		{
			err::out(stmt->getName(), {"variable '", stmt->getName().getDataStr(),
						   "' already exists in scope"});
			return false;
		}
	}
	if(val && !skip_val && val->getTy()->isVoid()) {
		err::out(stmt,
			 {"value expression returns void, which cannot be assigned to a var"});
		return false;
	}
	if(vtype && val && !skip_val &&
	   !vtype->getTy()->isCompatible(ctx, val->getTy(), stmt->getLoc()))
	{
		err::out(stmt, {"incompatible given type and value of the variable decl"});
		return false;
	}
	if(val && !skip_val && stmt->isComptime()) {
		if(!vpass.visit(val, &val) || !val->getVal()) {
			err::out(stmt, {"value of comptime variable could not be calculated"});
			return false;
		}
		val->setComptime();
	}
	if(val && !vtype) {
		if(val->getCast()) {
			stmt->setTyVal(val->getCast(), val->getVal());
		} else {
			stmt->setTyVal(val);
		}
	} else if(vtype) {
		stmt->setTy(vtype->getTy());
		if(stmt->isComptime() && !val) {
			stmt->setVal(vtype->getVal());
		}
	}
	if(!stmt->getTy()) {
		err::out(stmt, {"internal error: no type found in variable declaration"});
		return false;
	}
	if(vtype) stmt->appendStmtMask(vtype->getStmtMask());
	if(val) stmt->appendStmtMask(val->getStmtMask());
	// no checking if stmt had const or ref originally because let const/& is not allowed
	if(vtype && !vtype->isConst()) stmt->unsetConst();
	if(vtype && !vtype->isRef()) stmt->unsetRef();

	if(stmt->getVal() && !stmt->getVal()->isType()) {
		if(!stmt->isRef()) {
			stmt->setVal(stmt->getVal()->clone(ctx));
		} else if(stmt->getVal()->hasPermaData()) {
			err::out(stmt, {"a reference variable cannot have perma data"});
			return false;
		}
	}

	if(vtype && val && !skip_val) {
		if(val->isConst() != vtype->isConst()) {
			val->castTo(vtype->getTy(), vtype->getStmtMask());
		}
		if(!val->getCast()) applyPrimitiveTypeCoercion(vtype->getTy(), val);
	}
	if(stmt->getTy()->isFunc() && stmt->getVType()) {
		FuncTy *ty = as<FuncTy>(stmt->getTy());
		Stmt *s	   = stmt->getVType()->getExpr();
		if(!ty->getSig() && s && s->isFnSig()) {
			ty->setSig(as<StmtFnSig>(s));
		}
	}
	if(!stmt->isIn() && vmgr.exists(stmt->getName().getDataStr(), true, false)) {
		Type *t		 = vmgr.getTy(stmt->getName().getDataStr(), true, false);
		StructTy *destst = as<StructTy>(t);
		StructTy *srcst	 = as<StructTy>(stmt->getTy());
		if(destst->getTemplateNames() != srcst->getTemplateNames()) {
			err::out(stmt->getName(),
				 {"struct declaration templates don't match the definition"});
			return false;
		}
		for(size_t i = 0; i < srcst->getFields().size(); ++i) {
			destst->insertField(srcst->getFieldName(i), srcst->getField(i));
		}
		destst->setTemplates(srcst->getTemplates());
		destst->setDecl(srcst->getDecl());
		return deferredspecialize.specialize(destst->getID(), ctx,
						     stmt->getName().getLoc());
	}
	if(stmt->isIn()) {
		if(!stmt->getVal()) {
			err::out(stmt, {"internal error: function variable must have a Value"});
			return false;
		}
		StmtFnDef *def = as<StmtFnDef>(stmt->getVVal());
		StmtVar *self  = def->getSigArgs()[0];
		return vmgr.addTypeFn(self->getTy(), stmt->getName().getDataStr(),
				      as<FuncVal>(stmt->getVal()));
	}
	// since FuncTy contains the variable declaration in itself,
	// no need to pass this stmt to vmgr.addVar()
	return vmgr.addVar(stmt->getName().getDataStr(), stmt->getTy(), stmt->getVal(),
			   stmt->getTy()->isFunc() ? nullptr : stmt, stmt->isGlobal());
}
bool TypeAssignPass::visit(StmtFnSig *stmt, Stmt **source)
{
	auto &args		  = stmt->getArgs();
	disabled_varname_mangling = true;
	for(size_t i = 0; i < args.size(); ++i) {
		if(!visit(args[i], asStmt(&args[i]))) {
			err::out(stmt, {"failed to determine type of argument"});
			return false;
		}
	}
	if(!visit(stmt->getRetType(), asStmt(&stmt->getRetType()))) {
		err::out(stmt, {"failed to determine type of return type"});
		return false;
	}
	disabled_varname_mangling = false;
	Vector<Type *> argst;
	Vector<bool> argcomptime;
	for(auto &a : args) {
		argst.push_back(a->getTy());
		argcomptime.push_back(a->isComptime());
	}
	Type *retty = stmt->getRetType()->getTy();
	FuncTy *ft  = FuncTy::get(ctx, nullptr, argst, retty, argcomptime, nullptr, INONE, false,
				  stmt->hasVariadic());
	stmt->setTyVal(ft, FuncVal::create(ctx, ft));
	// needs to be executed to set the correct value for disable_template variable
	stmt->requiresTemplateInit();
	return true;
}
bool TypeAssignPass::visit(StmtFnDef *stmt, Stmt **source)
{
	pushFunc(nullptr, false, 0); // functy is set later
	if(!visit(stmt->getSig(), asStmt(&stmt->getSig()))) {
		err::out(stmt, {"failed to determine type of func signature"});
		return false;
	}
	FuncVal *fn   = as<FuncVal>(stmt->getSig()->getVal());
	FuncTy *sigty = fn->getVal();

	sigty->setVar(stmt->getParentVar());

	vmgr.getTopFunc().setTy(sigty);

	if(stmt->getParentVar()) {
		StringRef name = stmt->getParentVar()->getName().getDataStr();
		vmgr.addVar(name, stmt->getSig()->getTy(), stmt->getSig()->getVal(),
			    stmt->getParentVar());
	}

	if(stmt->requiresTemplateInit()) goto end;

	stmt->getBlk()->setTy(sigty->getRet());
	if(!visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) {
		err::out(stmt, {"failed to determine type of function block"});
		return false;
	}
end:
	stmt->setTyVal(stmt->getSig());
	popFunc();
	return true;
}
bool TypeAssignPass::visit(StmtHeader *stmt, Stmt **source) { return true; }
bool TypeAssignPass::visit(StmtLib *stmt, Stmt **source) { return true; }
bool TypeAssignPass::visit(StmtExtern *stmt, Stmt **source)
{
	if(stmt->getHeaders() && !visit(stmt->getHeaders(), asStmt(&stmt->getHeaders()))) {
		err::out(stmt, {"failed to assign header type"});
		return false;
	}
	if(stmt->getLibs() && !visit(stmt->getLibs(), asStmt(&stmt->getLibs()))) {
		err::out(stmt, {"failed to assign lib type"});
		return false;
	}
	if(!stmt->getEntity()) return true;
	vmgr.pushLayer();
	if(stmt->getEntity()->isStructDef()) {
		as<StmtStruct>(stmt->getEntity())->setExterned(true);
	}
	if(!visit(stmt->getEntity(), &stmt->getEntity())) {
		err::out(stmt, {"failed to determine type of extern entity"});
		return false;
	}
	if(stmt->getEntity()->isFnSig()) {
		FuncVal *fn = as<FuncVal>(stmt->getEntity()->getVal());
		fn->getVal()->setExterned(true);
		fn->getVal()->setVar(stmt->getParentVar());
	}
	stmt->setTyVal(stmt->getEntity());
	vmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtEnum *stmt, Stmt **source)
{
	const ModuleLoc *loc  = stmt->getLoc();
	static size_t enum_id = 0;
	StringRef enum_mangle = ctx.strFrom({"enum_", ctx.strFrom(enum_id++)});
	NamespaceVal *ns      = NamespaceVal::create(ctx, enum_mangle);
	size_t i	      = 0;
	Type *ty	      = nullptr;
	if(stmt->getTagTy() && !visit(stmt->getTagTy(), asStmt(&stmt->getTagTy()))) {
		err::out(stmt, {"failed to determine provided tag type for enum"});
		return false;
	}
	if(stmt->getTagTy() && !stmt->getTagTy()->getVal()->isType()) {
		err::out(stmt, {"expected enum tag type to be a type"});
		return false;
	}
	if(stmt->getTagTy() && !stmt->getTagTy()->getTy()->isInt()) {
		err::out(stmt, {"expected enum tag type to be an integer type, found: ",
				stmt->getTagTy()->getTy()->toStr()});
		return false;
	}
	if(stmt->getTagTy()) ty = stmt->getTagTy()->getTy();
	else ty = IntTy::get(ctx, 32, true);
	for(auto &e : stmt->getItems()) {
		e.setDataStr(getMangledName(stmt, e.getDataStr(), ns));
		Stmt *val    = StmtSimple::create(ctx, loc, lex::Lexeme(loc, (int64_t)i));
		StmtVar *var = StmtVar::create(ctx, loc, e, nullptr, val, 0);
		var->setTy(ty);
		var->setVal(IntVal::create(ctx, CDPERMA, i++));
		var->setComptime();
		var->setAppliedModuleID(true);
		additionalvars.push_back(var);
		vmgr.addVar(e.getDataStr(), var->getTy(), var->getVal(), var);
	}
	stmt->setTyVal(PtrTy::getStr(ctx), ns);
	addEnumTagTy(enum_mangle, ty);
	return true;
}
bool TypeAssignPass::visit(StmtStruct *stmt, Stmt **source)
{
	vmgr.pushLayer();
	Vector<TypeTy *> templates;
	Vector<StringRef> templatenames = stmt->getTemplateNames();

	disabled_varname_mangling = true;
	for(auto &t : templatenames) {
		templates.push_back(TypeTy::get(ctx));
		TypeVal *v = TypeVal::create(ctx, templates.back());
		vmgr.addVar(t, v->getVal(), v, nullptr);
	}

	StructTy *st =
	StructTy::get(ctx, stmt, {}, {}, templatenames, templates, stmt->isExterned());
	stmt->setTypeVal(ctx, st);
	vmgr.addVar("Self", stmt->getTy(), stmt->getVal(), nullptr);
	for(auto &f : stmt->getFields()) {
		if(!visit(f, asStmt(&f))) {
			err::out(stmt, {"failed to determine type of struct field"});
			return false;
		}
		st->insertField(f->getName().getDataStr(), f->getTy());
	}
	disabled_varname_mangling = false;
	vmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(auto &d : stmt->getDecls()) {
		if(!visit(d, asStmt(&d))) {
			err::out(stmt, {"failed to determine type of this variable declaration"});
			return false;
		}
	}
	return true;
}
bool TypeAssignPass::visit(StmtCond *stmt, Stmt **source)
{
	// TODO: erase inline if nothing exists in block
	// (for example, when condition never becomes true)
	for(auto &cond : stmt->getConditionals()) {
		Stmt *&c	= cond.getCond();
		StmtBlock *&b	= cond.getBlk();
		bool this_is_it = false;
		if(c && !visit(c, &c)) {
			err::out(stmt, {"failed to determine type of conditional"});
			return false;
		}
		if(c && !c->getTy()->isPrimitive()) {
			err::out(stmt, {"conditional expression type must be primitive"});
			return false;
		}
		if(!stmt->isInline() && !visit(b, asStmt(&b))) {
			err::out(stmt, {"failed to determine type in conditional block"});
			return false;
		}
		if(!stmt->isInline()) continue;
		if(!c) goto end;
		if(!vpass.visit(c, &c) || !c->getVal()) {
			err::out(stmt, {"failed to get condition value for inline conditional"});
			return false;
		}
		if(!c->getVal()->hasData()) {
			err::out(stmt, {"inline condition received no value"});
			return false;
		}
		if(c->getVal()->isInt()) {
			this_is_it = as<IntVal>(c->getVal())->getVal() != 0;
		}
		if(c->getVal()->isFlt()) {
			this_is_it = as<FltVal>(c->getVal())->getVal() != 0.0;
		}
		if(!this_is_it) continue;
	end:
		// no vmgr.(push/pop)Layer() if the inline conditional is at top level
		if(vmgr.isTop()) b->disableLayering();
		if(!visit(b, asStmt(&b))) {
			err::out(stmt, {"failed to determine types in inline conditional block"});
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
bool TypeAssignPass::visit(StmtFor *stmt, Stmt **source)
{
	// TODO: erase inline loop if nothing exists in block
	// (for example, when inline if is present whose condition never becomes true)
	if(stmt->isInline() && !stmt->getCond()) {
		err::out(stmt, {"inline for-loop requires a condition"});
		return false;
	}

	Stmt *&init	    = stmt->getInit();
	Stmt *&cond	    = stmt->getCond();
	Stmt *&incr	    = stmt->getIncr();
	StmtBlock *&blk	    = stmt->getBlk();
	StmtBlock *finalblk = nullptr;
	Vector<Stmt *> newblkstmts;

	vmgr.pushLayer();
	if(init && !visit(init, &init)) {
		err::out(stmt, {"failed to determine type of init expression in for loop"});
		return false;
	}
	if(cond && !visit(cond, &cond)) {
		err::out(stmt, {"failed to determine type of cond expression in for loop"});
		return false;
	}
	if(incr && !visit(incr, &incr)) {
		err::out(stmt, {"failed to determine type of incr expression in for loop"});
		return false;
	}
	if(cond && !cond->getTy()->isPrimitive()) {
		err::out(stmt, {"inline for-loop's condition must be a primitive (int/flt)"});
		return false;
	}

	if(!stmt->isInline() && blk && !visit(blk, asStmt(&blk))) {
		err::out(stmt, {"failed to determine type of for-loop block"});
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
		err::out(stmt, {"failed to determine value of inline for-loop init expr"});
		return false;
	}
	if(!vpass.visit(cond, &cond) || !cond->getVal()) {
		err::out(stmt, {"failed to determine value of inline for-loop condition;"
				" ensure relevant variables are comptime"});
		return false;
	}
	if(init) newblkstmts.push_back(init->clone(ctx));
	while((cond->getVal()->isInt() && as<IntVal>(cond->getVal())->getVal()) ||
	      (cond->getVal()->isFlt() && as<FltVal>(cond->getVal())->getVal()))
	{
		for(auto &s : blk->getStmts()) {
			newblkstmts.push_back(s->clone(ctx));
		}
		if(incr) newblkstmts.push_back(incr->clone(ctx));
		if(incr && !vpass.visit(incr, &incr)) {
			err::out(stmt, {"failed to determine value of inline for-loop incr"});
			return false;
		}
		if(!vpass.visit(cond, &cond)) {
			err::out(stmt, {"failed to determine value of inline for-loop condition"});
			return false;
		}
	}
	blk->getStmts() = newblkstmts;
	finalblk	= blk;
	stmt->getBlk()	= nullptr;
	*source		= finalblk;
	vmgr.popLayer();
	if(!visit(*source, source)) {
		err::out(*source, {"failed to determine type of inlined for-loop block"});
		return false;
	}
	(*source)->clearValue();
	return true;
}
bool TypeAssignPass::visit(StmtRet *stmt, Stmt **source)
{
	if(!vmgr.hasFunc()) {
		err::out(stmt, {"return statements can be in functions only"});
		return false;
	}
	Stmt *&val = stmt->getRetVal();
	if(val && !visit(val, &val)) {
		err::out(stmt, {"failed to determine type of the return argument"});
		return false;
	}
	FuncTy *fn	 = vmgr.getTopFunc().getFuncTy();
	StmtBlock *fnblk = as<StmtFnDef>(fn->getVar()->getVVal())->getBlk();
	if(!fn->getVar()) {
		err::out(stmt, {"function type has no declaration"});
		return false;
	}
	Type *valtype = val ? val->getTy()->specialize(ctx) : VoidTy::get(ctx);
	bool was_any  = false;
	if(fn->getRet()->isAny()) {
		Type *newr = valtype->specialize(ctx);
		fn->setRet(newr);
		fnblk->setTy(newr);
		if(val && fn->getSig()) {
			fn->getSig()->getRetType()->appendStmtMask(val->getStmtMask());
		}
		was_any = true;
	}
	Type *fnretty = fn->getRet();
	if(val && fn->getSig()) stmt->appendStmtMask(fn->getSig()->getRetType()->getStmtMask());
	if(!was_any && !fnretty->isCompatible(ctx, valtype, stmt->getLoc())) {
		err::out(stmt,
			 {"function return type '", fnretty->toStr(), "' and deduced return type '",
			  valtype->toStr(), "' are incompatible"});
		return false;
	}
	stmt->setFnBlk(fnblk);
	stmt->setTyVal(stmt->getFnBlk());
	if(val && !val->getCast() && fnretty->requiresCast(valtype)) {
		val->castTo(fnretty, fn->getSig() ? fn->getSig()->getRetType()->getStmtMask() : 0);
	}
	return true;
}
bool TypeAssignPass::visit(StmtContinue *stmt, Stmt **source) { return true; }
bool TypeAssignPass::visit(StmtBreak *stmt, Stmt **source) { return true; }
bool TypeAssignPass::visit(StmtDefer *stmt, Stmt **source)
{
	deferstack.addStmt(stmt->getDeferVal());
	*source = nullptr;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// Extra ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TypeAssignPass::getMangledName(Stmt *stmt, StringRef name, NamespaceVal *ns) const
{
	if(stmt->isSimple()) {
		StmtSimple *sim = as<StmtSimple>(stmt);
		if(sim->isAppliedModuleID()) return name;
	}
	return ctx.strFrom({name, "_", (ns ? ns->getVal() : stmt->getMod()->getID())});
}

void TypeAssignPass::applyPrimitiveTypeCoercion(Type *to, Stmt *from)
{
	if(!to || !from) return;
	if(!to->isPrimitiveOrPtr() || !from->getTy()->isPrimitiveOrPtr()) return;

	if(!to->requiresCast(from->getTy())) return;
	from->castTo(to, (uint8_t)0);
}

void TypeAssignPass::applyPrimitiveTypeCoercion(Stmt *lhs, Stmt *rhs, const lex::Lexeme &oper)
{
	if(!lhs || !rhs) return;

	Type *l = lhs->getTy();
	Type *r = rhs->getTy();

	if(!l->isPrimitiveOrPtr() || !r->isPrimitiveOrPtr()) return;
	if(oper.getTokVal() == lex::SUBS) return;

	if(l->getID() == r->getID()) return;

	if(oper.getTok().isAssign()) {
		if(r->isPtr() && !l->isPtr()) return;
		rhs->castTo(l->specialize(ctx), lhs->getStmtMask());
		rhs->unsetCastRef();
		return;
	}
	// 0 => lhs
	// 1 => rhs
	bool superior = chooseSuperiorPrimitiveType(l, r);
	if(superior) {
		rhs->castTo(l->specialize(ctx), lhs->getStmtMask());
	} else {
		lhs->castTo(r->specialize(ctx), rhs->getStmtMask());
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

bool TypeAssignPass::initTemplateFunc(Stmt *caller, FuncTy *&cf, Vector<Stmt *> &args)
{
	static Map<StringRef, StmtVar *> alreadytemplated;
	static Map<StringRef, StmtVar *> beingtemplated;
	// nothing to do if function has no definition
	if(!cf->getVar() || !cf->getVar()->getVVal()) return true;
	StmtVar *&cfvar = cf->getVar();
	if(!cfvar) return true;
	if(!cfvar->getVVal()->requiresTemplateInit()) {
		return true;
	}
	// semiunique because cf is yet to be modified according to variadics and such
	// in the loop below
	StringRef semiuniqname =
	ctx.strFrom({cfvar->getName().getDataStr(), ctx.strFrom(cf->getSignatureID())});
	if(beingtemplated.find(semiuniqname) != beingtemplated.end()) {
		cf = as<FuncTy>(beingtemplated[semiuniqname]->getTy());
		return true;
	}

	StmtBlock *cfblk = nullptr;
	// disable cloning of blk till necessary
	if(cfvar->getVVal()->isFnDef()) {
		StmtFnDef *cfdef = as<StmtFnDef>(cfvar->getVVal());
		cfblk		 = cfdef->getBlk();
		cfdef->setBlk(nullptr);
	}
	StmtVar *origcfvar = cfvar;
	cfvar		   = as<StmtVar>(cfvar->clone(ctx)); // template must be cloned
	StmtFnSig *cfsig   = nullptr;
	cf->setVar(cfvar);
	if(cfvar->getVVal()->isFnDef()) {
		StmtFnDef *cfdef = as<StmtFnDef>(cfvar->getVVal());
		cfdef->setParentVar(cfvar);
		cfsig = cfdef->getSig();
	} else if(cfvar->getVVal()->isExtern()) {
		StmtExtern *cfext = as<StmtExtern>(cfvar->getVVal());
		cfext->setParentVar(cfvar);
		cfsig = as<StmtFnSig>(cfext->getEntity());
	}
	cfsig->disableTemplates();
	cfsig->setVariadic(false);

	pushFunc(nullptr, false, 0); // data set a bit later

	bool is_va	= false;
	size_t va_count = 0;
	bool no_va_arg	= cf->getArgs().size() > 0 && cf->getArgs().back()->isVariadic() &&
			 args.size() < cf->getArgs().size();
	for(size_t i = 0; i < args.size() + no_va_arg; ++i) {
		StmtVar *cfa = cfsig->getArg(i);
		Type *cft    = cf->getArg(i);
		if(!cft->isVariadic()) {
			Type *cftc = cft->isAny() ? args[i]->getTy() : cft;
			cfa->setTyVal(args[i]->getTy(), args[i]->getVal());
			if(args[i]->getCast()) {
				cfa->castTo(args[i]->getCast(), args[i]->getCastStmtMask());
			}
			cf->setArg(i, cfa->getTy());
			vmgr.addVar(cfa->getName().getDataStr(), cfa->getTy(), cfa->getVal(), cfa);
			continue;
		}
		const ModuleLoc *mloc = cfa->getLoc();
		lex::Lexeme &va_name  = cfa->getName();
		Type *vaty	      = cft;
		is_va		      = true;
		cfsig->getArgs().pop_back();
		cf->getArgs().pop_back();
		bool has_val = false;
		for(auto &a : args) {
			if(a->getVal()) {
				has_val = true;
				break;
			}
		}
		VecVal *vv = nullptr;
		if(has_val) vv = VecVal::create(ctx, CDFALSE, {});
		while(i < args.size()) {
			StringRef argn =
			ctx.strFrom({va_name.getDataStr(), "__", ctx.strFrom(va_count)});
			StmtVar *newv = as<StmtVar>(cfa->clone(ctx));
			newv->getVType()->unsetVariadic();
			newv->getName().setDataStr(argn);
			Type *t = args[i]->getTy()->specialize(ctx);
			newv->setTyVal(t, args[i]->getVal());
			if(has_val) vv->insertVal(newv->getVal());
			cfsig->insertArg(newv);
			cf->insertArg(t);
			vmgr.addVar(argn, newv->getTy(), newv->getVal(), newv);
			++va_count;
			++i;
		}
		vmgr.addVar(va_name.getDataStr(), cft, vv, cfa);
		break;
	}
	cf->setVariadic(false);
	FuncVal *cfn = FuncVal::create(ctx, cf);
	cfsig->getRetType()->setTypeVal(ctx, cf->getRet());
	cfsig->setTyVal(cfn->getVal(), cfn);
	cfvar->getVVal()->setTyVal(cfsig);
	cfvar->setTyVal(cfsig);

	StringRef uniqname =
	ctx.strFrom({cfvar->getName().getDataStr(), ctx.strFrom(cf->getSignatureID())});
	if(alreadytemplated.find(uniqname) != alreadytemplated.end()) {
		if(cfblk) as<StmtFnDef>(origcfvar->getVVal())->setBlk(cfblk);
		cf = as<FuncTy>(alreadytemplated[uniqname]->getTy());
		popFunc();
		return true;
	}

	if(cf->isExtern()) {
		goto end;
	}
	if(cfblk) {
		as<StmtFnDef>(origcfvar->getVVal())->setBlk(cfblk);
		cfblk = as<StmtBlock>(cfblk->clone(ctx));
		as<StmtFnDef>(cfvar->getVVal())->setBlk(cfblk);
	}
	if(!cfblk) {
		err::out(caller, {"function definition for specialization has no block"});
		return false;
	}
	beingtemplated[uniqname] = cfvar;
	cfblk->setTy(cf->getRet());
	updateLastFunc(cfn, is_va, va_count);
	if(!visit(cfblk, asStmt(&cfblk))) {
		err::out(caller, {"failed to assign type for called template function's var"});
		return false;
	}
	cfsig->getRetType()->setTy(cf->getRet());
	beingtemplated.erase(uniqname);
	alreadytemplated[uniqname] = cfvar;
end:
	popFunc();

	additionalvars.push_back(cfvar);
	return true;
}

void TypeAssignPass::pushFunc(FuncVal *fn, bool is_va, size_t va_len)
{
	vmgr.pushFunc(fn ? fn->getVal() : nullptr);
	vmgr.pushLayer();
	deferstack.pushFunc();
	is_fn_va.push_back(is_va);
	valen.push_back(va_len);
}
void TypeAssignPass::updateLastFunc(FuncVal *fn, bool is_va, size_t va_len)
{
	vmgr.getTopFunc().setTy(fn->getVal());
	is_fn_va.back() = is_va;
	valen.back()	= va_len;
}
void TypeAssignPass::popFunc()
{
	deferstack.popFunc();
	vmgr.popLayer();
	vmgr.popFunc();
	is_fn_va.pop_back();
	valen.pop_back();
}

} // namespace sc