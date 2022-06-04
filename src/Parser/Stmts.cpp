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

#include "TreeIO.hpp"

namespace sc
{
Map<uint64_t, Value *> values = {{0, nullptr}};
uint64_t genValueID()
{
	static uint64_t vid = 1;
	return vid++;
}
uint64_t createValueIDWith(Value *v)
{
	uint64_t id = genValueID();
	values[id]  = v;
	return id;
}
Value *getValWithID(uint64_t id)
{
	auto loc = values.find(id);
	if(loc == values.end()) return nullptr;
	return loc->second;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(const Stmts &stmt_type, const ModuleLoc *loc)
	: loc(loc), ty(nullptr), val(nullptr), cast_to(nullptr), derefcount(0), stype(stmt_type),
	  stmtmask(0), castmask(0)
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
	case FOR: return "for loop";
	case RET: return "return";
	case CONTINUE: return "continue";
	case BREAK: return "break";
	case DEFER: return "defer";
	}
	return "";
}

String Stmt::getTypeString()
{
	if(!getTy() && !getVal()) return "";
	String res;
	res += " :: ";
	if(isComptime()) res += "comptime ";
	if(isRef()) res += "& ";
	if(isConst()) res += "const ";
	res += getTy()->toStr();
	if(cast_to) {
		res += " -> ";
		if(isCastComptime()) res += "comptime ";
		if(isCastRef()) res += "& ";
		if(isCastConst()) res += "const ";
		res += cast_to->toStr();
	}
	if(getVal() && getVal()->hasData()) res += " ==> " + getVal()->toStr();
	return res;
}
Type *&Stmt::getTy(bool exact)
{
	if(exact) return ty;
	if(cast_to) return cast_to;
	if(!derefcount) return ty;
	uint16_t tmp = derefcount;
	Type *t	     = ty;
	while(tmp-- > 1) {
		t = as<PtrTy>(t)->getTo();
	}
	return as<PtrTy>(t)->getTo();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBlock ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBlock::StmtBlock(const ModuleLoc *loc, const Vector<Stmt *> &stmts, bool is_top,
		     bool disable_layering)
	: Stmt(BLOCK, loc), stmts(stmts), is_top(is_top), disable_layering(disable_layering)
{}
StmtBlock::~StmtBlock() {}
StmtBlock *StmtBlock::create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &stmts,
			     bool is_top, bool disable_layering)
{
	return c.allocStmt<StmtBlock>(loc, stmts, is_top, disable_layering);
}

void StmtBlock::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Block [top = ", is_top ? "yes" : "no", "] [disable layering = ",
			      disable_layering ? "yes" : "no", "]:", getTypeString(), "\n"});
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!stmts[i]) {
			tio::taba(has_next);
			tio::print(i != stmts.size() - 1, {"<Source End>\n"});
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

StmtType::StmtType(const ModuleLoc *loc, const size_t &ptr, bool variadic, Stmt *expr)
	: Stmt(TYPE, loc), ptr(ptr), variadic(variadic), expr(expr)
{}
StmtType::~StmtType() {}
StmtType *StmtType::create(Context &c, const ModuleLoc *loc, const size_t &ptr, bool variadic,
			   Stmt *expr)
{
	return c.allocStmt<StmtType>(loc, ptr, variadic, expr);
}

void StmtType::disp(bool has_next)
{
	String tname(ptr, '*');
	if(variadic) tname = "..." + tname;

	if(!tname.empty()) {
		tio::taba(has_next);
		tio::print(has_next || expr, {"Type info: ", tname, getTypeString(), "\n"});
		tio::tabr();
	}
	tio::taba(has_next);
	tio::print(has_next, {"Type Expr:\n"});
	expr->disp(false);
	tio::tabr();
}

bool StmtType::requiresTemplateInit()
{
	if(variadic) return true;
	return expr->requiresTemplateInit();
}

String StmtType::getStringName()
{
	String tname(ptr, '*');
	if(variadic) tname = "..." + tname;
	tname += expr->getStmtTypeString();
	return tname;
}

bool StmtType::isMetaType() const
{
	return expr && expr->getStmtType() == SIMPLE &&
	       as<StmtSimple>(expr)->getLexValue().getTok() == lex::TYPE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSimple::StmtSimple(const ModuleLoc *loc, const lex::Lexeme &val)
	: Stmt(SIMPLE, loc), decl(nullptr), val(val), self(nullptr), applied_module_id(false)
{}

StmtSimple::~StmtSimple() {}
StmtSimple *StmtSimple::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &val)
{
	return c.allocStmt<StmtSimple>(loc, val);
}

void StmtSimple::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Simple [decl = ", decl ? "yes" : "no", "] [self = ",
			      self ? "yes" : "no", "]: ", val.str(0), getTypeString(), "\n"});
	tio::tabr();
}

bool StmtSimple::requiresTemplateInit()
{
	if(val.getTokVal() == lex::ANY || val.getTokVal() == lex::TYPE) return true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// StmtFnCallInfo ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnCallInfo::StmtFnCallInfo(const ModuleLoc *loc, const Vector<Stmt *> &args)
	: Stmt(FNCALLINFO, loc), args(args)
{}
StmtFnCallInfo::~StmtFnCallInfo() {}
StmtFnCallInfo *StmtFnCallInfo::create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &args)
{
	return c.allocStmt<StmtFnCallInfo>(loc, args);
}

void StmtFnCallInfo::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Function Call Info: ", args.empty() ? "(empty)" : "", "\n"});
	if(!args.empty()) {
		tio::taba(false);
		tio::print(false, {"Args:\n"});
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

StmtExpr::StmtExpr(const ModuleLoc *loc, const size_t &commas, Stmt *lhs, const lex::Lexeme &oper,
		   Stmt *rhs, bool is_intrinsic_call)
	: Stmt(EXPR, loc), commas(commas), lhs(lhs), oper(oper), rhs(rhs), or_blk(nullptr),
	  or_blk_var(loc), is_intrinsic_call(is_intrinsic_call), calledfn(nullptr)
{}
StmtExpr::~StmtExpr() {}
StmtExpr *StmtExpr::create(Context &c, const ModuleLoc *loc, const size_t &commas, Stmt *lhs,
			   const lex::Lexeme &oper, Stmt *rhs, bool is_intrinsic_call)
{
	return c.allocStmt<StmtExpr>(loc, commas, lhs, oper, rhs, is_intrinsic_call);
}

void StmtExpr::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Expression [parsing intinsic: ", is_intrinsic_call ? "yes" : "no",
			      "]:", getTypeString(), "\n"});
	if(lhs) {
		tio::taba(oper.getTok().isValid() || rhs || or_blk);
		tio::print(oper.getTok().isValid() || rhs || or_blk, {"LHS:\n"});
		lhs->disp(false);
		tio::tabr();
	}
	if(oper.getTok().isValid()) {
		tio::taba(rhs || or_blk);
		tio::print(rhs || or_blk, {"Oper: ", oper.getTok().cStr(), "\n"});
		tio::tabr();
	}
	if(rhs) {
		tio::taba(or_blk);
		tio::print(or_blk, {"RHS:\n"});
		rhs->disp(false);
		tio::tabr();
	}
	if(or_blk) {
		tio::taba(false);
		StringRef orblkvardata =
		or_blk_var.getTok().isData() ? or_blk_var.getDataStr() : "<none>";
		tio::print(false, {"Or: ", orblkvardata, "\n"});
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

StmtVar::StmtVar(const ModuleLoc *loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval,
		 uint8_t varmask)
	: Stmt(VAR, loc), name(name), vtype(vtype), vval(vval), varmask(varmask),
	  applied_module_id(false), applied_codegen_mangle(false)
{
	if(vtype) appendStmtMask(vtype->getStmtMask());
}
StmtVar::~StmtVar() {}
StmtVar *StmtVar::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &name, StmtType *vtype,
			 Stmt *vval, uint8_t infomask)
{
	return c.allocStmt<StmtVar>(loc, name, vtype, vval, infomask);
}

void StmtVar::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(
	has_next,
	{"Variable [in = ", isIn() ? "yes" : "no", "] [comptime = ", isComptime() ? "yes" : "no",
	 "] [global = ", isGlobal() ? "yes" : "no", "] [static = ", isStatic() ? "yes" : "no",
	 "] [const = ", isConst() ? "yes" : "no", "] [volatile = ", isVolatile() ? "yes" : "no",
	 "]: ", name.getDataStr(), getTypeString(), "\n"});
	if(vtype) {
		tio::taba(vval);
		tio::print(vval, {"Type:\n"});
		vtype->disp(false);
		tio::tabr();
	}
	if(vval) {
		tio::taba(false);
		tio::print(false, {"Value:\n"});
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

StmtFnSig::StmtFnSig(const ModuleLoc *loc, Vector<StmtVar *> &args, StmtType *rettype,
		     bool has_variadic)
	: Stmt(FNSIG, loc), args(args), rettype(rettype), disable_template(false),
	  has_variadic(has_variadic)
{}
StmtFnSig::~StmtFnSig() {}
StmtFnSig *StmtFnSig::create(Context &c, const ModuleLoc *loc, Vector<StmtVar *> &args,
			     StmtType *rettype, bool has_variadic)
{
	return c.allocStmt<StmtFnSig>(loc, args, rettype, has_variadic);
}

void StmtFnSig::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Function signature [variadic = ", has_variadic ? "yes" : "no", "]",
			      getTypeString(), "\n"});
	if(args.size() > 0) {
		tio::taba(rettype);
		tio::print(rettype, {"Parameters:\n"});
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(i != args.size() - 1);
		}
		tio::tabr();
	}
	if(rettype) {
		tio::taba(false);
		tio::print(false, {"Return Type", rettype->getTypeString(), "\n"});
		rettype->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtFnSig::requiresTemplateInit()
{
	if(disable_template) return false;
	if(has_variadic) return true;
	for(auto &a : args) {
		if(a->getTy()->isTemplate()) return true;
		if(a->isComptime()) return true;
		Type *t = a->getTy();
		while(t->isPtr()) t = as<PtrTy>(t)->getTo();
		if(t->isAny()) return true;
	}
	if(rettype->getTy()->isTemplate()) return true;
	Type *t = rettype->getTy();
	while(t->isPtr()) t = as<PtrTy>(t)->getTo();
	if(t->isAny()) return true;
	disable_template = true;
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnDef::StmtFnDef(const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk)
	: Stmt(FNDEF, loc), sig(sig), blk(blk), parentvar(nullptr), used(0)
{}
StmtFnDef::~StmtFnDef() {}
StmtFnDef *StmtFnDef::create(Context &c, const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk)
{
	return c.allocStmt<StmtFnDef>(loc, sig, blk);
}

void StmtFnDef::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Function definition [has parent: ", parentvar ? "yes" : "no", "]",
			      getTypeString(), "\n"});
	tio::taba(true);
	tio::print(true, {"Function Signature:\n"});
	sig->disp(false);
	tio::tabr();

	tio::taba(false);
	tio::print(false, {"Function Block:\n"});
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

StmtHeader::StmtHeader(const ModuleLoc *loc, const lex::Lexeme &names, const lex::Lexeme &flags)
	: Stmt(HEADER, loc), names(names), flags(flags)
{}
StmtHeader *StmtHeader::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &names,
			       const lex::Lexeme &flags)
{
	return c.allocStmt<StmtHeader>(loc, names, flags);
}

void StmtHeader::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Header\n"});
	tio::taba(!flags.getDataStr().empty());
	tio::print(!flags.getDataStr().empty(), {"Names: ", names.getDataStr(), "\n"});
	tio::tabr();

	if(!flags.getDataStr().empty()) {
		tio::taba(false);
		tio::print(false, {"Flags: ", flags.getDataStr(), "\n"});
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

StmtLib::StmtLib(const ModuleLoc *loc, const lex::Lexeme &flags) : Stmt(LIB, loc), flags(flags) {}
StmtLib *StmtLib::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &flags)
{
	return c.allocStmt<StmtLib>(loc, flags);
}

void StmtLib::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Libs\n"});
	tio::taba(false);
	tio::print(false, {"Flags: ", flags.getDataStr(), "\n"});
	tio::tabr(2);
}

bool StmtLib::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtExtern /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExtern::StmtExtern(const ModuleLoc *loc, const lex::Lexeme &fname, StmtHeader *headers,
		       StmtLib *libs, Stmt *entity)
	: Stmt(EXTERN, loc), fname(fname), headers(headers), libs(libs), entity(entity),
	  parentvar(nullptr)
{}
StmtExtern::~StmtExtern() {}
StmtExtern *StmtExtern::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &fname,
			       StmtHeader *headers, StmtLib *libs, Stmt *entity)
{
	return c.allocStmt<StmtExtern>(loc, fname, headers, libs, entity);
}

void StmtExtern::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Extern for ", fname.getDataStr(), " [has parent: ",
			      parentvar ? "yes" : "no", "]", getTypeString(), "\n"});
	if(headers) {
		tio::taba(libs || entity);
		tio::print(libs || entity, {"Headers:\n"});
		headers->disp(false);
		tio::tabr();
	}
	if(libs) {
		tio::taba(entity);
		tio::print(entity, {"Libs:\n"});
		libs->disp(false);
		tio::tabr();
	}

	if(entity) {
		tio::taba(false);
		tio::print(false, {"Entity:\n"});
		entity->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

bool StmtExtern::requiresTemplateInit()
{
	return entity->requiresTemplateInit();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// StmtEnum //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtEnum::StmtEnum(const ModuleLoc *loc, const Vector<lex::Lexeme> &items, StmtType *tagty)
	: Stmt(ENUMDEF, loc), items(items), tagty(tagty)
{}
StmtEnum::~StmtEnum() {}
StmtEnum *StmtEnum::create(Context &c, const ModuleLoc *loc, const Vector<lex::Lexeme> &items,
			   StmtType *tagty)
{
	return c.allocStmt<StmtEnum>(loc, items, tagty);
}

void StmtEnum::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Enumerations:", getTypeString(), "\n"});
	if(tagty) {
		tio::taba(!items.empty());
		tio::print(!items.empty(), {"Provided Tag Type:\n"});
		tagty->disp(false);
		tio::tabr();
	}
	for(size_t i = 0; i < items.size(); ++i) {
		tio::taba(i != items.size() - 1);
		tio::print(i != items.size() - 1, {items[i].str(0), "\n"});
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

StmtStruct::StmtStruct(const ModuleLoc *loc, const Vector<StmtVar *> &fields,
		       const Vector<lex::Lexeme> &templates, bool is_decl)
	: Stmt(STRUCTDEF, loc), fields(fields), templates(templates), is_decl(is_decl),
	  is_externed(false)
{}
StmtStruct::~StmtStruct() {}
StmtStruct *StmtStruct::create(Context &c, const ModuleLoc *loc, const Vector<StmtVar *> &fields,
			       const Vector<lex::Lexeme> &templates, bool is_decl)
{
	return c.allocStmt<StmtStruct>(loc, fields, templates, is_decl);
}

void StmtStruct::disp(bool has_next)
{
	String templatestr;
	if(!templates.empty()) {
		templatestr += "<";
		for(auto &t : templates) {
			templatestr += t.getDataStr();
			templatestr += ", ";
		}
		templatestr.pop_back();
		templatestr.pop_back();
		templatestr += ">";
	}

	tio::taba(has_next);
	tio::print(has_next,
		   {"Struct", templatestr, is_decl ? " (decl) " : " ", getTypeString(), "\n"});

	if(!fields.empty()) {
		tio::taba(false);
		tio::print(false, {"Fields:\n"});
		for(size_t i = 0; i < fields.size(); ++i) {
			fields[i]->disp(i != fields.size() - 1);
		}
		tio::tabr();
	}

	tio::tabr();
}

bool StmtStruct::requiresTemplateInit()
{
	if(templates.size() > 0) return true;
	for(auto &f : fields) {
		if(f->requiresTemplateInit() || f->isComptime()) return true;
	}
	return false;
}

Vector<StringRef> StmtStruct::getTemplateNames()
{
	Vector<StringRef> templatenames;
	for(auto &t : templates) {
		templatenames.push_back(t.getDataStr());
	}
	return templatenames;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVarDecl::StmtVarDecl(const ModuleLoc *loc, const Vector<StmtVar *> &decls)
	: Stmt(VARDECL, loc), decls(decls)
{}
StmtVarDecl::~StmtVarDecl() {}
StmtVarDecl *StmtVarDecl::create(Context &c, const ModuleLoc *loc, const Vector<StmtVar *> &decls)
{
	return c.allocStmt<StmtVarDecl>(loc, decls);
}

void StmtVarDecl::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Variable declarations\n"});
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

StmtCond::StmtCond(const ModuleLoc *loc, const Vector<Conditional> &conds, bool is_inline)
	: Stmt(COND, loc), conds(conds), is_inline(is_inline)
{}
StmtCond::~StmtCond() {}
StmtCond *StmtCond::create(Context &c, const ModuleLoc *loc, const Vector<Conditional> &conds,
			   bool is_inline)
{
	return c.allocStmt<StmtCond>(loc, conds, is_inline);
}

void StmtCond::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Conditional [is inline = ", is_inline ? "yes" : "no", "]\n"});
	for(size_t i = 0; i < conds.size(); ++i) {
		tio::taba(i != conds.size() - 1);
		tio::print(i != conds.size() - 1, {"Branch:\n"});
		if(conds[i].getCond()) {
			tio::taba(true);
			tio::print(true, {"Condition:\n"});
			conds[i].getCond()->disp(false);
			tio::tabr();
		}
		tio::taba(false);
		tio::print(false, {"Block:\n"});
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
//////////////////////////////////////////// StmtFor //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFor::StmtFor(const ModuleLoc *loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk,
		 bool is_inline)
	: Stmt(FOR, loc), init(init), cond(cond), incr(incr), blk(blk), is_inline(is_inline)
{}
StmtFor::~StmtFor() {}
StmtFor *StmtFor::create(Context &c, const ModuleLoc *loc, Stmt *init, Stmt *cond, Stmt *incr,
			 StmtBlock *blk, bool is_inline)
{
	return c.allocStmt<StmtFor>(loc, init, cond, incr, blk, is_inline);
}

void StmtFor::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"For/While [is inline = ", is_inline ? "yes" : "no", "]\n"});
	if(init) {
		tio::taba(cond || incr || blk);
		tio::print(cond || incr || blk, {"Init:\n"});
		init->disp(false);
		tio::tabr();
	}
	if(cond) {
		tio::taba(incr || blk);
		tio::print(incr || blk, {"Condition:\n"});
		cond->disp(false);
		tio::tabr();
	}
	if(incr) {
		tio::taba(blk);
		tio::print(blk, {"Increment:\n"});
		incr->disp(false);
		tio::tabr();
	}
	if(blk) {
		tio::taba(false);
		tio::print(false, {"Block:\n"});
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
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtRet::StmtRet(const ModuleLoc *loc, Stmt *val) : Stmt(RET, loc), val(val) {}
StmtRet::~StmtRet() {}
StmtRet *StmtRet::create(Context &c, const ModuleLoc *loc, Stmt *val)
{
	return c.allocStmt<StmtRet>(loc, val);
}

void StmtRet::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Return", getTypeString(), "\n"});
	if(val) {
		tio::taba(false);
		tio::print(false, {"Value:\n"});
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

StmtContinue::StmtContinue(const ModuleLoc *loc) : Stmt(CONTINUE, loc) {}
StmtContinue *StmtContinue::create(Context &c, const ModuleLoc *loc)
{
	return c.allocStmt<StmtContinue>(loc);
}

void StmtContinue::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Continue\n"});
	tio::tabr();
}

bool StmtContinue::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBreak ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBreak::StmtBreak(const ModuleLoc *loc) : Stmt(BREAK, loc) {}
StmtBreak *StmtBreak::create(Context &c, const ModuleLoc *loc)
{
	return c.allocStmt<StmtBreak>(loc);
}

void StmtBreak::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Break\n"});
	tio::tabr();
}

bool StmtBreak::requiresTemplateInit()
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtDefer ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtDefer::StmtDefer(const ModuleLoc *loc, Stmt *val) : Stmt(DEFER, loc), val(val) {}
StmtDefer::~StmtDefer() {}
StmtDefer *StmtDefer::create(Context &c, const ModuleLoc *loc, Stmt *val)
{
	return c.allocStmt<StmtDefer>(loc, val);
}

void StmtDefer::disp(bool has_next)
{
	tio::taba(has_next);
	tio::print(has_next, {"Defer", getTypeString(), "\n"});
	if(val) {
		tio::taba(false);
		tio::print(false, {"Value:\n"});
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