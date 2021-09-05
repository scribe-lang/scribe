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

// #include "parser/Type.hpp"
// #include "parser/ValueMgr.hpp"
#include "TreeIO.hpp"

namespace sc
{
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(const Stmts &stmt_type, const ModuleLoc &loc)
	: stype(stmt_type), loc(loc), tv(nullptr, nullptr)
{}
Stmt::~Stmt()
{
	// if(type) delete type;
	// if(cast_from) delete cast_from;
}

std::string Stmt::getStmtTypeString() const
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
	return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBlock ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBlock::StmtBlock(const ModuleLoc &loc, const std::vector<Stmt *> &stmts)
	: Stmt(BLOCK, loc), stmts(stmts)
{}
StmtBlock::~StmtBlock()
{
	for(auto &stmt : stmts) {
		if(!stmt) continue;
		delete stmt;
	}
}

void StmtBlock::StmtBlock::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Block:\n");
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtType /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtType::StmtType(const ModuleLoc &loc, const size_t &ptr, const size_t &info,
		   const std::vector<Stmt *> &array_counts, const std::vector<lex::Lexeme> &name,
		   const std::vector<lex::Lexeme> &templates)
	: Stmt(TYPE, loc), ptr(ptr), info(info), array_counts(array_counts), name(name),
	  templates(templates), fn(nullptr)
{}
StmtType::StmtType(const ModuleLoc &loc, const std::vector<Stmt *> &array_counts, Stmt *fn)
	: Stmt(TYPE, loc), ptr(0), info(0), array_counts(array_counts), fn(fn)
{}
StmtType::~StmtType()
{
	for(auto &ac : array_counts) delete ac;
	if(fn) delete fn;
}

void StmtType::disp(const bool &has_next) const
{
	if(fn) {
		tio::taba(!array_counts.empty() || has_next);
		tio::print(!array_counts.empty() || has_next, "Type: <Function>%s\n",
			   getTypeString().c_str());
		fn->disp(false);
		if(array_counts.size() > 0) {
			tio::tabr();
			tio::taba(has_next);
			tio::print(has_next, "Array Counts:\n");
			for(size_t i = 0; i < array_counts.size(); ++i) {
				array_counts[i]->disp(i != array_counts.size() - 1);
			}
		}
		tio::tabr();
		return;
	}
	std::string tname(ptr, '*');
	if(info & REF) tname += "&";
	if(info & STATIC) tname += "static ";
	if(info & CONST) tname += "const ";
	if(info & VOLATILE) tname += "volatile ";
	if(info & VARIADIC) tname = "..." + tname;
	for(auto &n : name) tname += n.getDataStr().empty() ? n.getTok().cStr() : n.getDataStr();
	if(!templates.empty()) {
		tname += "<";
		for(auto &t : templates) {
			tname += t.getDataStr() + ", ";
		}
		tname.pop_back();
		tname.pop_back();
		tname += ">";
	}
	tio::taba(!array_counts.empty() || has_next);
	tio::print(!array_counts.empty() || has_next, "Type: %s%s\n", tname.c_str(),
		   getTypeString().c_str());
	if(array_counts.size() > 0) {
		tio::tabr();
		tio::taba(has_next);
		tio::print(has_next, "Array Counts:\n");
		for(size_t i = 0; i < array_counts.size(); ++i) {
			array_counts[i]->disp(i != array_counts.size() - 1);
		}
	}
	tio::tabr();
}

bool StmtType::hasModifier(const size_t &tim) const
{
	return info & tim;
}

std::string StmtType::getStringName()
{
	if(fn) return fn->getStmtTypeString();

	std::string tname(ptr, '*');
	if(info & REF) tname += "&";
	if(info & STATIC) tname += "static ";
	if(info & CONST) tname += "const ";
	if(info & VOLATILE) tname += "volatile ";
	if(info & VARIADIC) tname = "..." + tname;
	for(auto &n : name) tname += n.getDataStr().empty() ? n.getTok().cStr() : n.getDataStr();
	if(templates.empty()) return tname;
	tname += "<";
	for(auto &t : templates) {
		tname += t.getDataStr();
	}
	tname += ">";
	return tname;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSimple::StmtSimple(const ModuleLoc &loc, const lex::Lexeme &val) : Stmt(SIMPLE, loc), val(val)
{}

void StmtSimple::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Simple: %s%s\n", val.str(0).c_str(), getTypeString().c_str());
	tio::tabr();
}

StmtSimple::~StmtSimple() {}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// StmtFnCallInfo ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnCallInfo::StmtFnCallInfo(const ModuleLoc &loc, const std::vector<StmtType *> &templates,
			       const std::vector<Stmt *> &args)
	: Stmt(FNCALLINFO, loc), templates(templates), args(args)
{}
StmtFnCallInfo::~StmtFnCallInfo()
{
	for(auto &templ : templates) delete templ;
	for(auto &a : args) delete a;
}

void StmtFnCallInfo::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Function Call Info: %s\n",
		   templates.empty() && args.empty() ? "(empty)" : "");
	if(!templates.empty()) {
		tio::taba(!args.empty());
		tio::print(!args.empty(), "Template Types:\n");
		for(size_t i = 0; i < templates.size(); ++i) {
			templates[i]->disp(i != templates.size() - 1);
		}
		tio::tabr();
	}
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExpr::StmtExpr(const ModuleLoc &loc, const size_t &commas, Stmt *lhs, const lex::Lexeme &oper,
		   Stmt *rhs, const bool &is_intrinsic_call)
	: Stmt(EXPR, loc), commas(commas), lhs(lhs), oper(oper), rhs(rhs), or_blk(nullptr),
	  or_blk_var(loc), is_intrinsic_call(is_intrinsic_call)
{}
StmtExpr::~StmtExpr()
{
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	if(or_blk) delete or_blk;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVar::StmtVar(const ModuleLoc &loc, const lex::Lexeme &name, StmtType *vtype, Stmt *val,
		 const bool &is_comptime)
	: Stmt(VAR, loc), name(name), vtype(vtype), val(val), is_comptime(is_comptime)
{}
StmtVar::~StmtVar()
{
	if(vtype) delete vtype;
	if(val) delete val;
}

void StmtVar::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Variable [comptime = %s]: %s%s\n", is_comptime ? "yes" : "no",
		   name.getDataStr().c_str(), getTypeString().c_str());
	if(vtype) {
		tio::taba(val);
		tio::print(val, "Type:\n");
		vtype->disp(false);
		tio::tabr();
	}
	if(val) {
		tio::taba(false);
		tio::print(false, "Value:\n");
		val->disp(false);
		tio::tabr();
	}
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnSig ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnSig::StmtFnSig(const ModuleLoc &loc, const std::vector<lex::Lexeme> &templates,
		     std::vector<StmtVar *> &args, StmtType *rettype, const bool &has_variadic,
		     const bool &is_member)
	: Stmt(FNSIG, loc), templates(templates), args(args), rettype(rettype),
	  has_variadic(has_variadic), is_member(is_member)
{}
StmtFnSig::~StmtFnSig()
{
	for(auto &p : args) delete p;
	if(rettype) delete rettype;
}

void StmtFnSig::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Function signature [variadic = %s, member = %s, templates = %zu]%s\n",
		   has_variadic ? "yes" : "no", is_member ? "yes" : "no", templates.size(),
		   getTypeString().c_str());
	if(!templates.empty()) {
		tio::taba(args.size() > 0 || rettype);
		tio::print(args.size() > 0 || rettype, "Templates:\n");
		for(size_t i = 0; i < templates.size(); ++i) {
			tio::taba(i != templates.size() - 1);
			tio::print(i != templates.size() - 1, "%s\n",
				   templates[i].getDataStr().c_str());
			tio::tabr();
		}
		tio::tabr();
	}
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnDef::StmtFnDef(const ModuleLoc &loc, StmtFnSig *sig, StmtBlock *blk)
	: Stmt(FNDEF, loc), sig(sig), blk(blk)
{}
StmtFnDef::~StmtFnDef()
{
	if(sig) delete sig;
	if(blk) delete blk;
}

void StmtFnDef::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Function definition%s\n", getTypeString().c_str());
	tio::taba(true);
	tio::print(true, "Function Signature:\n");
	sig->disp(false);
	tio::tabr();

	tio::taba(false);
	tio::print(false, "Function Block:\n");
	blk->disp(false);
	tio::tabr(2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtHeader /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtHeader::StmtHeader(const ModuleLoc &loc, const lex::Lexeme &names, const lex::Lexeme &flags)
	: Stmt(HEADER, loc), names(names), flags(flags)
{}

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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtLib //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtLib::StmtLib(const ModuleLoc &loc, const lex::Lexeme &flags) : Stmt(LIB, loc), flags(flags) {}

void StmtLib::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Libs\n");
	tio::taba(false);
	tio::print(false, "Flags: %s\n", flags.getDataStr().c_str());
	tio::tabr(2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtExtern /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExtern::StmtExtern(const ModuleLoc &loc, const lex::Lexeme &fname, StmtHeader *headers,
		       StmtLib *libs, StmtFnSig *sig)
	: Stmt(EXTERN, loc), fname(fname), headers(headers), libs(libs), sig(sig)
{}
StmtExtern::~StmtExtern()
{
	if(headers) delete headers;
	if(libs) delete libs;
	if(sig) delete sig;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// StmtEnum //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtEnum::StmtEnum(const ModuleLoc &loc, const std::vector<lex::Lexeme> &items)
	: Stmt(ENUMDEF, loc), items(items)
{}
StmtEnum::~StmtEnum() {}

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

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtStruct //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtStruct::StmtStruct(const ModuleLoc &loc, const bool &decl,
		       const std::vector<lex::Lexeme> &templates,
		       const std::vector<StmtVar *> &fields)
	: Stmt(STRUCTDEF, loc), decl(decl), templates(templates), fields(fields)
{}
StmtStruct::~StmtStruct()
{
	for(auto &field : fields) delete field;
}

void StmtStruct::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Struct [declaration = %s]%s\n", decl ? "yes" : "no",
		   getTypeString().c_str());

	if(!templates.empty()) {
		tio::taba(!fields.empty());
		tio::print(!fields.empty(), "Templates:\n");
		for(size_t i = 0; i < templates.size(); ++i) {
			tio::taba(i != templates.size() - 1);
			tio::print(i != templates.size() - 1, "%s\n", templates[i].str(0).c_str());
			tio::tabr();
		}
		tio::tabr();
	}

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

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVarDecl::StmtVarDecl(const ModuleLoc &loc, const std::vector<StmtVar *> &decls)
	: Stmt(VARDECL, loc), decls(decls)
{}
StmtVarDecl::~StmtVarDecl()
{
	for(auto &decl : decls) delete decl;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Conditional::Conditional(Stmt *cond, StmtBlock *blk) : cond(cond), blk(blk) {}
Conditional::~Conditional() {}

StmtCond::StmtCond(const ModuleLoc &loc, const std::vector<Conditional> &conds,
		   const bool &is_inline)
	: Stmt(COND, loc), conds(conds), is_inline(is_inline)
{}
StmtCond::~StmtCond()
{
	for(auto &c : conds) {
		if(c.getCond()) delete c.getCond();
		if(c.getBlk()) delete c.getBlk();
	}
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

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtForIn //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtForIn::StmtForIn(const ModuleLoc &loc, const lex::Lexeme &iter, Stmt *in, StmtBlock *blk)
	: Stmt(FORIN, loc), iter(iter), in(in), blk(blk)
{}
StmtForIn::~StmtForIn()
{
	if(in) delete in;
	if(blk) delete blk;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFor //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFor::StmtFor(const ModuleLoc &loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk,
		 const bool &is_inline)
	: Stmt(FOR, loc), init(init), cond(cond), incr(incr), blk(blk), is_inline(is_inline)
{}
StmtFor::~StmtFor()
{
	if(init) delete init;
	if(cond) delete cond;
	if(incr) delete incr;
	if(blk) delete blk;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtWhile ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtWhile::StmtWhile(const ModuleLoc &loc, Stmt *cond, StmtBlock *blk)
	: Stmt(WHILE, loc), cond(cond), blk(blk)
{}
StmtWhile::~StmtWhile()
{
	if(cond) delete cond;
	if(blk) delete blk;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtRet::StmtRet(const ModuleLoc &loc, Stmt *val) : Stmt(RET, loc), val(val) {}
StmtRet::~StmtRet()
{
	if(val) delete val;
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

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtContinue ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtContinue::StmtContinue(const ModuleLoc &loc) : Stmt(CONTINUE, loc) {}

void StmtContinue::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Continue\n");
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBreak ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBreak::StmtBreak(const ModuleLoc &loc) : Stmt(BREAK, loc) {}

void StmtBreak::disp(const bool &has_next) const
{
	tio::taba(has_next);
	tio::print(has_next, "Break\n");
	tio::tabr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtDefer::StmtDefer(const ModuleLoc &loc, Stmt *val) : Stmt(DEFER, loc), val(val) {}
StmtDefer::~StmtDefer()
{
	if(val) delete val;
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

} // namespace sc