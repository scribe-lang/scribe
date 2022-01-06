/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Parser/Stmts.hpp"

namespace sc
{
///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBlock ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtBlock::clone(Context &ctx)
{
	std::vector<Stmt *> newstmts;
	for(auto &stmt : stmts) {
		newstmts.push_back(stmt->clone(ctx));
	}
	Stmt *res = StmtBlock::create(ctx, getLoc(), newstmts, is_top);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtType /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtType::clone(Context &ctx)
{
	Stmt *res = StmtType::create(ctx, getLoc(), ptr, info, expr->clone(ctx));
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtSimple::clone(Context &ctx)
{
	StmtSimple *newsim	  = StmtSimple::create(ctx, getLoc(), val);
	newsim->self		  = self ? self->clone(ctx) : nullptr;
	newsim->applied_module_id = applied_module_id;
	newsim->castTo(getCast());
	return newsim;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// StmtFnCallInfo ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtFnCallInfo::clone(Context &ctx)
{
	std::vector<Stmt *> newargs;
	for(auto &a : args) {
		newargs.push_back(a->clone(ctx));
	}
	Stmt *res = StmtFnCallInfo::create(ctx, getLoc(), newargs);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtExpr::clone(Context &ctx)
{
	StmtExpr *newexpr =
	StmtExpr::create(ctx, getLoc(), commas, lhs ? lhs->clone(ctx) : nullptr, oper,
			 rhs ? rhs->clone(ctx) : nullptr, is_intrinsic_call);
	newexpr->or_blk	    = or_blk ? as<StmtBlock>(or_blk->clone(ctx)) : nullptr;
	newexpr->or_blk_var = or_blk_var;
	newexpr->calledfn   = calledfn;
	newexpr->castTo(getCast());
	return newexpr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtVar::clone(Context &ctx)
{
	StmtType *newvtype = vtype ? as<StmtType>(vtype->clone(ctx)) : nullptr;
	Stmt *newvval	   = vval ? vval->clone(ctx) : nullptr;
	StmtVar *res =
	StmtVar::create(ctx, getLoc(), name, newvtype, newvval, is_in, is_comptime, is_global);
	res->setInfo(info);
	res->setAppliedModuleID(applied_module_id);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnSig ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtFnSig::clone(Context &ctx)
{
	std::vector<StmtVar *> newargs;
	for(auto &a : args) {
		newargs.push_back(as<StmtVar>(a->clone(ctx)));
	}
	StmtType *newrettype = rettype ? as<StmtType>(rettype->clone(ctx)) : nullptr;
	Stmt *res	     = StmtFnSig::create(ctx, getLoc(), newargs, newrettype, has_variadic);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtFnDef::clone(Context &ctx)
{
	StmtFnSig *newsig = as<StmtFnSig>(sig->clone(ctx));
	StmtBlock *newblk = blk ? as<StmtBlock>(blk->clone(ctx)) : nullptr;
	Stmt *res	  = StmtFnDef::create(ctx, getLoc(), newsig, newblk);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtHeader /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtHeader::clone(Context &ctx)
{
	Stmt *res = StmtHeader::create(ctx, getLoc(), names, flags);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtLib //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtLib::clone(Context &ctx)
{
	Stmt *res = StmtLib::create(ctx, getLoc(), flags);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtExtern /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtExtern::clone(Context &ctx)
{
	StmtHeader *newheaders = headers ? as<StmtHeader>(headers->clone(ctx)) : nullptr;
	StmtLib *newlibs       = libs ? as<StmtLib>(libs->clone(ctx)) : nullptr;
	Stmt *newent	       = entity->clone(ctx);
	Stmt *res = StmtExtern::create(ctx, getLoc(), fname, newheaders, newlibs, newent);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// StmtEnum //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtEnum::clone(Context &ctx)
{
	Stmt *res = StmtEnum::create(ctx, getLoc(), items);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtStruct //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtStruct::clone(Context &ctx)
{
	std::vector<StmtVar *> newfields;
	for(auto &f : fields) {
		newfields.push_back(as<StmtVar>(f->clone(ctx)));
	}
	Stmt *res = StmtStruct::create(ctx, getLoc(), newfields, templates);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtVarDecl::clone(Context &ctx)
{
	std::vector<StmtVar *> newdecls;
	for(auto &d : decls) {
		newdecls.push_back(as<StmtVar>(d->clone(ctx)));
	}
	Stmt *res = StmtVarDecl::create(ctx, getLoc(), newdecls);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtCond::clone(Context &ctx)
{
	std::vector<Conditional> newconds;
	for(auto &c : conds) {
		Stmt *newcond	  = c.getCond() ? c.getCond()->clone(ctx) : nullptr;
		StmtBlock *newblk = as<StmtBlock>(c.getBlk()->clone(ctx));
		newconds.push_back(Conditional(newcond, newblk));
	}
	Stmt *res = StmtCond::create(ctx, getLoc(), newconds, is_inline);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFor //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtFor::clone(Context &ctx)
{
	Stmt *newinit	  = init ? init->clone(ctx) : nullptr;
	Stmt *newcond	  = cond ? cond->clone(ctx) : nullptr;
	Stmt *newincr	  = incr ? incr->clone(ctx) : nullptr;
	StmtBlock *newblk = blk ? as<StmtBlock>(blk->clone(ctx)) : nullptr;
	Stmt *res = StmtFor::create(ctx, getLoc(), newinit, newcond, newincr, newblk, is_inline);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtWhile ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtWhile::clone(Context &ctx)
{
	Stmt *newcond	  = cond ? cond->clone(ctx) : nullptr;
	StmtBlock *newblk = blk ? as<StmtBlock>(blk->clone(ctx)) : nullptr;
	Stmt *res	  = StmtWhile::create(ctx, getLoc(), newcond, newblk);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtRet::clone(Context &ctx)
{
	Stmt *newval = val ? val->clone(ctx) : nullptr;
	StmtRet *res = StmtRet::create(ctx, getLoc(), newval);
	res->setFnBlk(fnblk);
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtContinue ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtContinue::clone(Context &ctx)
{
	Stmt *res = StmtContinue::create(ctx, getLoc());
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBreak ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtBreak::clone(Context &ctx)
{
	Stmt *res = StmtBreak::create(ctx, getLoc());
	res->castTo(getCast());
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtDefer ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt *StmtDefer::clone(Context &ctx)
{
	Stmt *res = StmtDefer::create(ctx, getLoc(), val->clone(ctx));
	res->castTo(getCast());
	return res;
}

} // namespace sc