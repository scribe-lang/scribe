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
TypeAssignPass::TypeAssignPass(ErrMgr &err, Context &ctx) : Pass(err, ctx) {}
TypeAssignPass::~TypeAssignPass() {}

bool TypeAssignPass::visit(Stmt *stmt, Stmt **source)
{
	switch(stmt->getStmtType()) {
	case BLOCK: visit(as<StmtBlock>(stmt), source); return true;
	case TYPE: visit(as<StmtType>(stmt), source); return true;
	case SIMPLE: visit(as<StmtSimple>(stmt), source); return true;
	case EXPR: visit(as<StmtExpr>(stmt), source); return true;
	case FNCALLINFO: visit(as<StmtFnCallInfo>(stmt), source); return true;
	case VAR: visit(as<StmtVar>(stmt), source); return true;
	case FNSIG: visit(as<StmtFnSig>(stmt), source); return true;
	case FNDEF: visit(as<StmtFnDef>(stmt), source); return true;
	case HEADER: visit(as<StmtHeader>(stmt), source); return true;
	case LIB: visit(as<StmtLib>(stmt), source); return true;
	case EXTERN: visit(as<StmtExtern>(stmt), source); return true;
	case ENUMDEF: visit(as<StmtEnum>(stmt), source); return true;
	case STRUCTDEF: visit(as<StmtStruct>(stmt), source); return true;
	case VARDECL: visit(as<StmtVarDecl>(stmt), source); return true;
	case COND: visit(as<StmtCond>(stmt), source); return true;
	case FORIN: visit(as<StmtForIn>(stmt), source); return true;
	case FOR: visit(as<StmtFor>(stmt), source); return true;
	case WHILE: visit(as<StmtWhile>(stmt), source); return true;
	case RET: visit(as<StmtRet>(stmt), source); return true;
	case CONTINUE: visit(as<StmtContinue>(stmt), source); return true;
	case BREAK: visit(as<StmtBreak>(stmt), source); return true;
	case DEFER: visit(as<StmtDefer>(stmt), source); return true;
	}
	err.set(stmt, "invalid statement foudn for type assignment: %s",
		stmt->getStmtTypeCString());
	return false;
}

bool TypeAssignPass::visit(StmtBlock *stmt, Stmt **source)
{
	tmgr.pushLayer();
	auto &stmts = stmt->getStmts();
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!visit(stmts[i], &stmts[i])) {
			err.set(stmt, "failed to assign type to stmt in block");
			return false;
		}
	}
	tmgr.popLayer();
	return true;
}
bool TypeAssignPass::visit(StmtType *stmt, Stmt **source)
{
	std::unordered_map<std::string, Type *> resolvabletemplates;
	size_t j = 0;
	for(auto &t : stmt->getTemplates()) {
		if(tmgr.exists(t.getDataStr(), false, true)) {
			Type *ty = tmgr.getTy(t.getDataStr(), false, true);
			resolvabletemplates[std::to_string(j++)] = ty;
			continue;
		}
		std::string tname = std::to_string(j++);
		tmgr.addVar(t.getDataStr(), TemplTy::create(ctx, tname), stmt);
	}
	if(stmt->isFunc()) {
		if(!visit(stmt->getFunc(), &stmt->getFunc())) {
			err.set(stmt, "failed to determine type of function type");
			return false;
		}
		stmt->setType(stmt->getFunc()->getType());
		return true;
	}
	Type *res = nullptr;
	if(stmt->hasModifier(VARIADIC) && (res = tmgr.getTy(stmt->getStringName(), false, true))) {
		res = res->clone(ctx);
		res->setInfo(stmt->getInfoMask());
		res->unsetVariadic();
		for(size_t i = 0; i < stmt->getPtrCount(); ++i) {
			res = PtrTy::create(ctx, res);
		}
		return res;
	}
	std::vector<lex::Lexeme> &names = stmt->getName();
	for(size_t i = 0; i < names.size(); ++i) {
		const std::string &name = names[i].getDataStr();
		if(!res) {
			std::string mangled = getMangledName(stmt, name);
			res		    = tmgr.getTy(mangled, false, true);
			if(res) names[i].setDataStr(mangled);
			if(!res && !(res = tmgr.getTy(name, false, true))) {
				err.set(stmt, "type %s does not exist", mangled.c_str());
				return false;
			}
			continue;
		}
		if(!res->isImport()) {
			err.set(names[i],
				"a type can only be at the outermost level, or inside an import");
			return false;
		}
		std::string mangled = getMangledName(stmt, name, as<ImportTy>(res));
		res		    = tmgr.getTy(mangled, false, false);
		if(!res) {
			err.set(stmt, "type %s does not exist in import", mangled.c_str());
			return false;
		}
		names[i].setDataStr(mangled);
	}
	res->setInfo(stmt->getInfoMask());

	// generate the array count part
	for(auto &c : stmt->getArrayCounts()) {
		if(!visit(c, &c)) {
			err.set(stmt, "failed to determine type of array count");
			return false;
		}
		IntVal *iv = as<IntVal>(c->getValue());
		if(!c->getType()->isIntegral()) {
			err.set(c, "index for an array must be integral");
			return false;
		}
		res = ArrayTy::create(ctx, iv->getVal(), res);
	}
	// generate ptrs
	for(size_t i = 0; i < stmt->getPtrCount(); ++i) {
		res = PtrTy::create(ctx, res);
	}
	if(resolvabletemplates.empty()) {
		stmt->setType(res);
	} else {
		std::unordered_set<std::string> unresolved_templates; // dummy
		stmt->setType(res->specialize(ctx, resolvabletemplates, unresolved_templates));
	}
	return true;
}
bool TypeAssignPass::visit(StmtSimple *stmt, Stmt **source)
{
	switch(stmt->getLexValue().getTok().getVal()) {
	case lex::TRUE:	 // fallthrough
	case lex::FALSE: // fallthrough
	case lex::NIL: stmt->setType(Int1Ty::create(ctx)); break;
	case lex::INT: stmt->setType(Int32Ty::create(ctx)); break;
	case lex::FLT: stmt->setType(Flt32Ty::create(ctx)); break;
	case lex::CHAR: stmt->setType(Int8Ty::create(ctx)); break;
	case lex::STR: {
		Type *ty = Int8Ty::create(ctx);
		ty->setConst();
		ty = ArrayTy::create(ctx, stmt->getLexValue().getDataStr().size(), ty);
		stmt->setType(ty);
		break;
	}
	case lex::IDEN: {
		Stmt *decl		 = nullptr;
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
		break;
	}
	default: return false;
	}
	if(!stmt->getType()) {
		err.set(stmt, "failed to determine type of this simple statement");
		return false;
	}
	return true;
}
bool TypeAssignPass::visit(StmtFnCallInfo *stmt, Stmt **source)
{
	tmgr.pushLayer();
	for(auto &t : stmt->getTemplates()) {
		if(!visit(t, asStmt(&t))) {
			err.set(stmt, "failed to determine type of template");
			return false;
		}
	}
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
			if(!visit(stmt->getRHS(), &stmt->getRHS())) {
				err.set(stmt, "failed to determine type of RHS in dot expression");
				return false;
			}
			rsim->updateLexDataStr(mangled);
			rsim->setAppliedModuleID(true);
			// replace this stmt with RHS (effectively removing LHS - import)
			*source = rhs;
			// once the source is changed, stmt will also be invalidated - therefore,
			// cannot be used anymore
			return true;
		}
		Type *res	      = nullptr;
		std::string fieldname = rsim->getLexValue().getDataStr();
		if(lhs->getType()->isStruct()) {
			StructTy *lst = as<StructTy>(lhs->getType());
			if(lst->isDef()) {
				err.set(stmt, "cannot use dot operator on a structure definition; "
					      "instantiate the struct first");
				return false;
			}
			// struct field names are not mangled with the module ids
			res = lst->getField(fieldname);
		}
		if(!res && !(res = tmgr.getTypeFn(lhs->getType(), fieldname))) {
			err.set(stmt, "no field or function '%s' in type '%s'", fieldname.c_str(),
				lhs->getType()->toStr().c_str());
			return false;
		}
		rhs->setType(res);
		stmt->setType(res);
		break;
	}
	case lex::FNCALL: {
		assert(rhs && rhs->getStmtType() == FNCALLINFO &&
		       "RHS must be function call info for a function call");
		if(!lhs->getType()->isFunc() && !lhs->getType()->isStruct()) {
			err.set(stmt,
				"function call can be performed only on functions or struct defs");
			return false;
		}
		StmtFnCallInfo *callinfo = as<StmtFnCallInfo>(rhs);
		std::unordered_map<std::string, Type *> templates;
		bool has_va = false;
		if(lhs->getType()->isFunc()) {
			FuncTy *fn = as<FuncTy>(lhs->getType());
			if(!(fn = fn->createCall(ctx, err, callinfo, templates))) {
				err.set(stmt, "function is incompatible with call arguments");
				return false;
			}
			has_va = fn->hasVariadic();
			lhs->setType(fn);
			stmt->setType(fn);
			if(stmt->isIntrinsicCall()) {
				if(!fn->isIntrinsic()) {
					err.set(stmt, "function call is intrinsic but the function "
						      "itself is not");
					return false;
				}
				if(!fn->callIntrinsic(ctx, stmt, source, callinfo)) {
					err.set(stmt, "call to intrinsic failed");
					return false;
				}
			} else if(fn->isIntrinsic()) {
				err.set(stmt, "function is intrinsic - required '@' before call");
				return false;
			}
			size_t fnarglen	  = fn->getArgs().size();
			size_t callarglen = callinfo->getArgs().size();
			for(size_t i = 0, j = 0, k = 0; i < fnarglen && j < callarglen; ++i, ++j) {
				Type *coerced_to = fn->getArg(i);
				if(coerced_to->isVariadic()) {
					coerced_to = as<VariadicTy>(coerced_to)->getArg(k++);
					--i;
				}
				applyPrimitiveTypeCoercion(coerced_to, callinfo->getArg(j));
			}
		} else if(lhs->getType()->isStruct()) {
			StructTy *st = as<StructTy>(lhs->getType());
			if(!st->isDef()) {
				err.set(stmt, "only struct definitions can be instantiated");
				return false;
			}
			if(!(st = st->instantiate(ctx, err, callinfo, templates))) {
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
			break;
		}
		// apply template specialization
		if(!initTemplateFunc(stmt, stmt->getType(), templates, has_va)) return false;
		break;
	}
	case lex::SUBS: {
		if(lhs->getType()->isVariadic()) {
			if(!rhs->getType()->isIntegral()) {
				err.set(rhs, "index for a variadic must be integral");
				return false;
			}
			// TODO:
			// if(!rhs->assignValue()) {}
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
		} else if(lhs->getType()->isArray()) {
			if(!rhs->getType()->isIntegral()) {
				err.set(rhs, "index for a pointer must be integral");
				return false;
			}
			stmt->setType(as<ArrayTy>(lhs->getType())->getOf());
		}
		goto applyoperfn;
	}
	// address of
	case lex::UAND: {
		stmt->setType(PtrTy::create(ctx, lhs->getType()));
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
		stmt->setType(fn->getRet());

		StmtFnCallInfo *callinfo = new StmtFnCallInfo(stmt->getLoc(), {}, {lhs});
		Pointer<StmtFnCallInfo> raiicallinfo(callinfo);
		if(rhs) callinfo->getArgs().push_back(rhs);
		if(!fn->callIntrinsic(ctx, stmt, source, callinfo)) {
			err.set(stmt, "call to intrinsic failed");
			return false;
		}
		std::unordered_map<std::string, Type *> calltemplates;
		bool erred;
		if(!initTemplateFunc(stmt, (Type *&)fn, calltemplates, false)) {
			erred = true;
			err.set(stmt, "failed to intialize template function");
			goto end;
		}
		for(auto &t : calltemplates) delete t.second;
		callinfo->getArgs().clear();
		delete fn;
	end:
		if(erred) return false;
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
	return true;
}
bool TypeAssignPass::visit(StmtFnSig *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtFnDef *stmt, Stmt **source)
{
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
	return true;
}
bool TypeAssignPass::visit(StmtEnum *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtStruct *stmt, Stmt **source)
{
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
	return true;
}
bool TypeAssignPass::visit(StmtForIn *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtFor *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtWhile *stmt, Stmt **source)
{
	return true;
}
bool TypeAssignPass::visit(StmtRet *stmt, Stmt **source)
{
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
	static std::unordered_map<int, int> prec = {
	{TI1, 0},  {TI8, 1},  {TU8, 2},	 {TI16, 3}, {TU16, 4},	{TI32, 5},
	{TU32, 6}, {TI64, 7}, {TU64, 8}, {TF32, 9}, {TF64, 10},
	};

	return prec[l->getID()] > prec[r->getID()];
}

bool TypeAssignPass::initTemplateFunc(Stmt *caller, Type *&calledfn,
				      std::unordered_map<std::string, Type *> &calltemplates,
				      const bool &has_va)
{
	if(calltemplates.empty() && !has_va) return true;
	assert(calledfn && "LHS has no type assigned");
	// nothing to do if function has no definition
	if(!calledfn->isFunc()) return true;
	FuncTy *cf = as<FuncTy>(calledfn);
	if(!cf->getDef()) return true;
	Stmt *calledfnblk = cf->getDef()->getParentWithType(BLOCK);
	if(!calledfnblk) {
		err.set(caller, "function definition for specialization is not in a block");
		return false;
	}
	Stmt *fndefvar	      = cf->getDef()->getParentWithType(VAR);
	static size_t spec_id = 1;
	if(!fndefvar->getSpecializedID()) {
		fndefvar->setSpecializedID(spec_id++);
	}
}
} // namespace sc