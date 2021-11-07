/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Parser/Stmts.hpp"

#include "TreeIO.hpp"

namespace sc
{
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(const Stmts &stmt_type, const ModuleLoc &loc)
	: stype(stmt_type), loc(loc), type(nullptr), cast_from(nullptr), value(nullptr),
	  is_value_perma(false)
{}
Stmt::~Stmt() {}

const char *Stmt::getStmtTypeCString() const
{
	switch(stype) {
	case BLOCK: return "block";
	case TYPE: return "type";
	case SIMPLE: return "simple";
	case EXPR: return "expression";
	case FNCALLINFO: return "function call info";
	case VAR: return "variable declaration base";
	case FNSIG: return "function signature";
	case FNDEF: return "function definition";
	case HEADER: return "extern header";
	case LIB: return "extern library";
	case EXTERN: return "extern";
	case ENUMDEF: return "enumeration definition";
	case STRUCTDEF: return "structure definition";
	case VARDECL: return "variable declaration";
	case COND: return "conditional";
	case FORIN: return "for-in loop";
	case FOR: return "for loop";
	case WHILE: return "while loop";
	case RET: return "return";
	case CONTINUE: return "continue";
	case BREAK: return "break";
	case DEFER: return "defer";
	}
	return "";
}

std::string Stmt::getTypeString() const
{
	if(!type) return "";
	std::string res = " :: " + type->toStr();
	if(cast_from) res += " <- " + cast_from->toStr();
	if(value) res += " ==> " + value->toStr();
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBlock ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBlock::StmtBlock(const ModuleLoc &loc, const std::vector<Stmt *> &stmts, const bool &is_top)
	: Stmt(BLOCK, loc), stmts(stmts), is_top(is_top)
{}
StmtBlock::~StmtBlock() {}
StmtBlock *StmtBlock::create(Context &c, const ModuleLoc &loc, const std::vector<Stmt *> &stmts,
			     const bool &is_top)
{
	return c.allocStmt<StmtBlock>(loc, stmts, is_top);
}

void StmtBlock::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Block [top = %s]:\n", is_top ? "yes" : "no");
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!stmts[i]) {
			tio::taba(has_next);
			tio::print(i != stmts.size() - 1, "<Source End>\n");
			tio::tabr();
			continue;
		}
		stmts[i]->disp(i != stmts.size() - 1);
	}
	tio::tabr();
}

bool StmtBlock::requiresTemplateInit()
{
	for(auto &s : stmts) {
		if(s->requiresTemplateInit()) return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtType /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtType::StmtType(const ModuleLoc &loc, const size_t &ptr, const size_t &info, Stmt *expr)
	: Stmt(TYPE, loc), ptr(ptr), info(info), expr(expr)
{}
StmtType::~StmtType() {}
StmtType *StmtType::create(Context &c, const ModuleLoc &loc, const size_t &ptr, const size_t &info,
			   Stmt *expr)
{
	return c.allocStmt<StmtType>(loc, ptr, info, expr);
}

void StmtType::disp(const bool &has_next) const
{
	std::string tname(ptr, '*');
	if(info & REF) tname += "&";
	if(info & STATIC) tname += "static ";
	if(info & CONST) tname += "const ";
	if(info & VOLATILE) tname += "volatile ";
	if(info & VARIADIC) tname = "..." + tname;
	if(!tname.empty()) {
		tio::taba(has_next);
		tio::print(has_next || expr, "Type info: %s%s\n", tname.c_str(),
			   getTypeString().c_str());
		tio::tabr();
	}
	tio::taba(has_next);
	tio::print(has_next, "Type Expr:\n");
	expr->disp(false);
	tio::tabr();
}

bool StmtType::requiresTemplateInit()
{
	if(info & VARIADIC) return true;
	return expr->requiresTemplateInit();
}

bool StmtType::hasModifier(const size_t &tim) const
{
	return info & tim;
}

std::string StmtType::getStringName()
{
	std::string tname(ptr, '*');
	if(info & REF) tname += "&";
	if(info & STATIC) tname += "static ";
	if(info & CONST) tname += "const ";
	if(info & VOLATILE) tname += "volatile ";
	if(info & VARIADIC) tname = "..." + tname;
	tname += expr->getStmtTypeString();
	return tname;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSimple::StmtSimple(const ModuleLoc &loc, const lex::Lexeme &val)
	: Stmt(SIMPLE, loc), decl(nullptr), val(val), self(nullptr), applied_module_id(false)
{}

StmtSimple::~StmtSimple() {}
StmtSimple *StmtSimple::create(Context &c, const ModuleLoc &loc, const lex::Lexeme &val)
{
	return c.allocStmt<StmtSimple>(loc, val);
}

void StmtSimple::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Simple [decl = %s] [self = %s]: %s%s\n", decl ? "yes" : "no",
		   self ? "yes" : "no", val.str(0).c_str(), getTypeString().c_str());
	tio::tabr();
}

bool StmtSimple::requiresTemplateInit()
{
	if(val.getTok().getVal() == lex::ANY || val.getTok().getVal() == lex::TYPE) return true;
	return decl ? decl->requiresTemplateInit() : false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// StmtFnCallInfo ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnCallInfo::StmtFnCallInfo(const ModuleLoc &loc, const std::vector<Stmt *> &args)
	: Stmt(FNCALLINFO, loc), args(args)
{}
StmtFnCallInfo::~StmtFnCallInfo() {}
StmtFnCallInfo *StmtFnCallInfo::create(Context &c, const ModuleLoc &loc,
				       const std::vector<Stmt *> &args)
{
	return c.allocStmt<StmtFnCallInfo>(loc, args);
}

void StmtFnCallInfo::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Function Call Info: %s\n", args.empty() ? "(empty)" : "");
	if(!args.empty()) {
		tio::taba(false);
		tio::print(false, "Args:\n");
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(i != args.size() - 1);
		}
		tio::tabr();
	}
	tio::tabr();
}

bool StmtFnCallInfo::requiresTemplateInit()
{
	for(auto &a : args) {
		if(a->requiresTemplateInit()) return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExpr::StmtExpr(const ModuleLoc &loc, const size_t &commas, Stmt *lhs, const lex::Lexeme &oper,
		   Stmt *rhs, const bool &is_intrinsic_call)
	: Stmt(EXPR, loc), commas(commas), lhs(lhs), oper(oper), rhs(rhs), or_blk(nullptr),
	  or_blk_var(loc), is_intrinsic_call(is_intrinsic_call), calledfn(nullptr)
{}
StmtExpr::~StmtExpr() {}
StmtExpr *StmtExpr::create(Context &c, const ModuleLoc &loc, const size_t &commas, Stmt *lhs,
			   const lex::Lexeme &oper, Stmt *rhs, const bool &is_intrinsic_call)
{
	return c.allocStmt<StmtExpr>(loc, commas, lhs, oper, rhs, is_intrinsic_call);
}

void StmtExpr::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Expression [parsing intinsic: %s]:%s\n",
		   is_intrinsic_call ? "yes" : "no", getTypeString().c_str());
	if(lhs) {
		tio::taba(oper.getTok().isValid() || rhs || or_blk);
		tio::print(oper.getTok().isValid() || rhs || or_blk, "LHS:\n");
		lhs->disp(false);
		tio::tabr();
	}
	if(oper.getTok().isValid()) {
		tio::taba(rhs || or_blk);
		tio::print(rhs || or_blk, "Oper: %s\n", oper.getTok().cStr());
		tio::tabr();
	}
	if(rhs) {
		tio::taba(or_blk);
		tio::print(or_blk, "RHS:\n");
		rhs->disp(false);
		tio::tabr();
	}
	if(or_blk) {
		tio::taba(false);
		tio::print(false, "Or: %s\n",
			   or_blk_var.getTok().isData() ? or_blk_var.getDataStr().c_str()
							: "<none>");
		or_blk->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtExpr::requiresTemplateInit()
{
	if(lhs && lhs->requiresTemplateInit()) return true;
	if(rhs && rhs->requiresTemplateInit()) return true;
	if(or_blk && or_blk->requiresTemplateInit()) return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVar::StmtVar(const ModuleLoc &loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval,
		 const bool &is_in, const bool &is_comptime, const bool &is_global)
	: Stmt(VAR, loc), name(name), is_in(is_in), vtype(vtype), vval(vval),
	  is_comptime(is_comptime), is_global(is_global), applied_module_id(false),
	  is_temp_vval(false)
{}
StmtVar::~StmtVar() {}
StmtVar *StmtVar::create(Context &c, const ModuleLoc &loc, const lex::Lexeme &name, StmtType *vtype,
			 Stmt *vval, const bool &is_in, const bool &is_comptime,
			 const bool &is_global)
{
	return c.allocStmt<StmtVar>(loc, name, vtype, vval, is_in, is_comptime, is_global);
}

void StmtVar::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Variable [in = %s] [comptime = %s] [global = %s]: %s%s\n",
		   is_in ? "yes" : "no", is_comptime ? "yes" : "no", is_global ? "yes" : "no",
		   name.getDataStr().c_str(), getTypeString().c_str());
	if(vtype) {
		tio::taba(vval);
		tio::print(vval, "Type:\n");
		vtype->disp(false);
		tio::tabr();
	}
	if(vval) {
		tio::taba(false);
		tio::print(false, "Value:\n");
		vval->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtVar::requiresTemplateInit()
{
	if(vtype && vtype->requiresTemplateInit()) return true;
	if(vval && vval->requiresTemplateInit()) return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnSig ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnSig::StmtFnSig(const ModuleLoc &loc, std::vector<StmtVar *> &args, StmtType *rettype,
		     const size_t &scope, const bool &has_variadic)
	: Stmt(FNSIG, loc), args(args), rettype(rettype), scope(scope), has_template(false),
	  has_variadic(has_variadic)
{}
StmtFnSig::~StmtFnSig() {}
StmtFnSig *StmtFnSig::create(Context &c, const ModuleLoc &loc, std::vector<StmtVar *> &args,
			     StmtType *rettype, const size_t &scope, const bool &has_variadic)
{
	return c.allocStmt<StmtFnSig>(loc, args, rettype, scope, has_variadic);
}

void StmtFnSig::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Function signature [variadic = %s]%s\n", has_variadic ? "yes" : "no",
		   getTypeString().c_str());
	if(args.size() > 0) {
		tio::taba(rettype);
		tio::print(rettype, "Parameters:\n");
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(i != args.size() - 1);
		}
		tio::tabr();
	}
	if(rettype) {
		tio::taba(false);
		tio::print(false, "Return Type:\n");
		rettype->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtFnSig::requiresTemplateInit()
{
	if(hasTemplate() || has_variadic) return true;
	for(auto &a : args) {
		if(a->getType() && a->getType()->isTemplate()) return true;
		if(a->isComptime()) return true;
	}
	if(rettype->getType() && rettype->getType()->isTemplate()) return true;
	return false;
}

bool StmtFnSig::hasTemplate()
{
	if(has_template) return true;
	for(auto &a : args) {
		if(!a->getType()->isTemplate()) continue;
		has_template = true;
		break;
	}
	if(rettype->getType()->isTemplate()) has_template = true;
	return has_template;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnDef::StmtFnDef(const ModuleLoc &loc, StmtFnSig *sig, StmtBlock *blk)
	: Stmt(FNDEF, loc), sig(sig), blk(blk), parentvar(nullptr)
{}
StmtFnDef::~StmtFnDef() {}
StmtFnDef *StmtFnDef::create(Context &c, const ModuleLoc &loc, StmtFnSig *sig, StmtBlock *blk)
{
	return c.allocStmt<StmtFnDef>(loc, sig, blk);
}

void StmtFnDef::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Function definition [has parent: %s]%s\n", parentvar ? "yes" : "no",
		   getTypeString().c_str());
	tio::taba(true);
	tio::print(true, "Function Signature:\n");
	sig->disp(false);
	tio::tabr();

	tio::taba(false);
	tio::print(false, "Function Block:\n");
	blk->disp(false);
	tio::tabr(2);
}

bool StmtFnDef::requiresTemplateInit()
{
	if(sig->requiresTemplateInit()) return true;
	if(blk && blk->requiresTemplateInit()) return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtHeader /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtHeader::StmtHeader(const ModuleLoc &loc, const lex::Lexeme &names, const lex::Lexeme &flags)
	: Stmt(HEADER, loc), names(names), flags(flags)
{}
StmtHeader *StmtHeader::create(Context &c, const ModuleLoc &loc, const lex::Lexeme &names,
			       const lex::Lexeme &flags)
{
	return c.allocStmt<StmtHeader>(loc, names, flags);
}

void StmtHeader::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Header\n");
	tio::taba(!flags.getDataStr().empty());
	tio::print(!flags.getDataStr().empty(), "Names: %s\n", names.getDataStr().c_str());
	tio::tabr();

	if(!flags.getDataStr().empty()) {
		tio::taba(false);
		tio::print(false, "Flags: %s\n", flags.getDataStr().c_str());
		tio::tabr();
	}
	tio::tabr();
}

bool StmtHeader::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtLib //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtLib::StmtLib(const ModuleLoc &loc, const lex::Lexeme &flags) : Stmt(LIB, loc), flags(flags) {}
StmtLib *StmtLib::create(Context &c, const ModuleLoc &loc, const lex::Lexeme &flags)
{
	return c.allocStmt<StmtLib>(loc, flags);
}

void StmtLib::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Libs\n");
	tio::taba(false);
	tio::print(false, "Flags: %s\n", flags.getDataStr().c_str());
	tio::tabr(2);
}

bool StmtLib::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtExtern /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExtern::StmtExtern(const ModuleLoc &loc, const lex::Lexeme &fname, StmtHeader *headers,
		       StmtLib *libs, StmtFnSig *sig)
	: Stmt(EXTERN, loc), fname(fname), headers(headers), libs(libs), sig(sig)
{}
StmtExtern::~StmtExtern() {}
StmtExtern *StmtExtern::create(Context &c, const ModuleLoc &loc, const lex::Lexeme &fname,
			       StmtHeader *headers, StmtLib *libs, StmtFnSig *sig)
{
	return c.allocStmt<StmtExtern>(loc, fname, headers, libs, sig);
}

void StmtExtern::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Extern for %s%s\n", fname.getDataStr().c_str(),
		   getTypeString().c_str());
	if(headers) {
		tio::taba(libs || sig);
		tio::print(libs || sig, "Headers:\n");
		headers->disp(false);
		tio::tabr();
	}
	if(libs) {
		tio::taba(sig);
		tio::print(sig, "Libs:\n");
		libs->disp(false);
		tio::tabr();
	}

	if(sig) {
		tio::taba(false);
		tio::print(false, "Funtion Signature:\n");
		sig->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtExtern::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// StmtEnum //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtEnum::StmtEnum(const ModuleLoc &loc, const std::vector<lex::Lexeme> &items)
	: Stmt(ENUMDEF, loc), items(items)
{}
StmtEnum::~StmtEnum() {}
StmtEnum *StmtEnum::create(Context &c, const ModuleLoc &loc, const std::vector<lex::Lexeme> &items)
{
	return c.allocStmt<StmtEnum>(loc, items);
}

void StmtEnum::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Enumerations:%s\n", getTypeString().c_str());
	for(size_t i = 0; i < items.size(); ++i) {
		tio::taba(i != items.size() - 1);
		tio::print(i != items.size() - 1, "%s\n", items[i].str(0).c_str());
		tio::tabr();
	}
	tio::tabr();
}

bool StmtEnum::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtStruct //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtStruct::StmtStruct(const ModuleLoc &loc, const std::vector<StmtVar *> &fields)
	: Stmt(STRUCTDEF, loc), fields(fields)
{}
StmtStruct::~StmtStruct() {}
StmtStruct *StmtStruct::create(Context &c, const ModuleLoc &loc,
			       const std::vector<StmtVar *> &fields)
{
	return c.allocStmt<StmtStruct>(loc, fields);
}

void StmtStruct::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Struct %s\n", getTypeString().c_str());

	if(!fields.empty()) {
		tio::taba(false);
		tio::print(false, "Fields:\n");
		for(size_t i = 0; i < fields.size(); ++i) {
			fields[i]->disp(i != fields.size() - 1);
		}
		tio::tabr();
	}

	tio::tabr();
}

bool StmtStruct::requiresTemplateInit()
{
	for(auto &f : fields) {
		if(f->requiresTemplateInit() || f->isComptime()) return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVarDecl::StmtVarDecl(const ModuleLoc &loc, const std::vector<StmtVar *> &decls)
	: Stmt(VARDECL, loc), decls(decls)
{}
StmtVarDecl::~StmtVarDecl() {}
StmtVarDecl *StmtVarDecl::create(Context &c, const ModuleLoc &loc,
				 const std::vector<StmtVar *> &decls)
{
	return c.allocStmt<StmtVarDecl>(loc, decls);
}

void StmtVarDecl::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Variable declarations\n");
	for(size_t i = 0; i < decls.size(); ++i) {
		decls[i]->disp(i != decls.size() - 1);
	}
	tio::tabr();
}

bool StmtVarDecl::requiresTemplateInit()
{
	for(auto &v : decls) {
		if(v->requiresTemplateInit()) return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Conditional::Conditional(Stmt *cond, StmtBlock *blk) : cond(cond), blk(blk) {}
Conditional::~Conditional() {}

StmtCond::StmtCond(const ModuleLoc &loc, const std::vector<Conditional> &conds,
		   const bool &is_inline)
	: Stmt(COND, loc), conds(conds), is_inline(is_inline)
{}
StmtCond::~StmtCond() {}
StmtCond *StmtCond::create(Context &c, const ModuleLoc &loc, const std::vector<Conditional> &conds,
			   const bool &is_inline)
{
	return c.allocStmt<StmtCond>(loc, conds, is_inline);
}

void StmtCond::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Conditional [is inline = %s]\n", is_inline ? "yes" : "no");
	for(size_t i = 0; i < conds.size(); ++i) {
		tio::taba(i != conds.size() - 1);
		tio::print(i != conds.size() - 1, "Branch:\n");
		if(conds[i].getCond()) {
			tio::taba(true);
			tio::print(true, "Condition:\n");
			conds[i].getCond()->disp(false);
			tio::tabr();
		}
		tio::taba(false);
		tio::print(false, "Block:\n");
		conds[i].getBlk()->disp(false);
		tio::tabr(2);
	}
	tio::tabr();
}

bool StmtCond::requiresTemplateInit()
{
	if(is_inline) return true;
	for(auto &c : conds) {
		if(c.getCond() && c.getCond()->requiresTemplateInit()) return true;
		if(c.getBlk() && c.getBlk()->requiresTemplateInit()) return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtForIn //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtForIn::StmtForIn(const ModuleLoc &loc, const lex::Lexeme &iter, Stmt *in, StmtBlock *blk)
	: Stmt(FORIN, loc), iter(iter), in(in), blk(blk)
{}
StmtForIn::~StmtForIn() {}
StmtForIn *StmtForIn::create(Context &c, const ModuleLoc &loc, const lex::Lexeme &iter, Stmt *in,
			     StmtBlock *blk)
{
	return c.allocStmt<StmtForIn>(loc, iter, in, blk);
}

void StmtForIn::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "For-in loop with iterator: %s:\n", iter.getDataStr().c_str());
	tio::taba(true);
	tio::print(true, "In:\n");
	in->disp(false);
	tio::tabr();
	tio::taba(false);
	tio::print(false, "Block:\n");
	blk->disp(false);
	tio::tabr(2);
}

bool StmtForIn::requiresTemplateInit()
{
	if(in->requiresTemplateInit()) return true;
	if(blk && blk->requiresTemplateInit()) return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFor //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFor::StmtFor(const ModuleLoc &loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk,
		 const bool &is_inline)
	: Stmt(FOR, loc), init(init), cond(cond), incr(incr), blk(blk), is_inline(is_inline)
{}
StmtFor::~StmtFor() {}
StmtFor *StmtFor::create(Context &c, const ModuleLoc &loc, Stmt *init, Stmt *cond, Stmt *incr,
			 StmtBlock *blk, const bool &is_inline)
{
	return c.allocStmt<StmtFor>(loc, init, cond, incr, blk, is_inline);
}

void StmtFor::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "For [is inline = %s]\n", is_inline ? "yes" : "no");
	if(init) {
		tio::taba(cond || incr || blk);
		tio::print(cond || incr || blk, "Init:\n");
		init->disp(false);
		tio::tabr();
	}
	if(cond) {
		tio::taba(incr || blk);
		tio::print(incr || blk, "Condition:\n");
		cond->disp(false);
		tio::tabr();
	}
	if(incr) {
		tio::taba(blk);
		tio::print(blk, "Increment:\n");
		incr->disp(false);
		tio::tabr();
	}
	if(blk) {
		tio::taba(false);
		tio::print(false, "Block:\n");
		blk->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtFor::requiresTemplateInit()
{
	if(is_inline) return true;
	if(blk && blk->requiresTemplateInit()) return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtWhile ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtWhile::StmtWhile(const ModuleLoc &loc, Stmt *cond, StmtBlock *blk)
	: Stmt(WHILE, loc), cond(cond), blk(blk)
{}
StmtWhile::~StmtWhile() {}
StmtWhile *StmtWhile::create(Context &c, const ModuleLoc &loc, Stmt *cond, StmtBlock *blk)
{
	return c.allocStmt<StmtWhile>(loc, cond, blk);
}

void StmtWhile::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "While\n");
	tio::taba(true);
	tio::print(true, "Condition:\n");
	cond->disp(true);
	tio::tabr();
	tio::taba(false);
	tio::print(false, "Block:\n");
	blk->disp(false);
	tio::tabr(2);
}

bool StmtWhile::requiresTemplateInit()
{
	if(blk && blk->requiresTemplateInit()) return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtRet::StmtRet(const ModuleLoc &loc, Stmt *val) : Stmt(RET, loc), val(val) {}
StmtRet::~StmtRet() {}
StmtRet *StmtRet::create(Context &c, const ModuleLoc &loc, Stmt *val)
{
	return c.allocStmt<StmtRet>(loc, val);
}

void StmtRet::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Return%s\n", getTypeString().c_str());
	if(val) {
		tio::taba(false);
		tio::print(false, "Value:\n");
		val->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtRet::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtContinue ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtContinue::StmtContinue(const ModuleLoc &loc) : Stmt(CONTINUE, loc) {}
StmtContinue *StmtContinue::create(Context &c, const ModuleLoc &loc)
{
	return c.allocStmt<StmtContinue>(loc);
}

void StmtContinue::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Continue\n");
	tio::tabr();
}

bool StmtContinue::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBreak ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBreak::StmtBreak(const ModuleLoc &loc) : Stmt(BREAK, loc) {}
StmtBreak *StmtBreak::create(Context &c, const ModuleLoc &loc)
{
	return c.allocStmt<StmtBreak>(loc);
}

void StmtBreak::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Break\n");
	tio::tabr();
}

bool StmtBreak::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtDefer ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtDefer::StmtDefer(const ModuleLoc &loc, Stmt *val) : Stmt(DEFER, loc), val(val) {}
StmtDefer::~StmtDefer() {}
StmtDefer *StmtDefer::create(Context &c, const ModuleLoc &loc, Stmt *val)
{
	return c.allocStmt<StmtDefer>(loc, val);
}

void StmtDefer::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Defer%s\n", getTypeString().c_str());
	if(val) {
		tio::taba(false);
		tio::print(false, "Value:\n");
		val->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtDefer::requiresTemplateInit()
{
	if(val->requiresTemplateInit()) return true;
	return false;
}

} // namespace sc