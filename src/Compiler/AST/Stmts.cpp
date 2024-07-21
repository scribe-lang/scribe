#include "AST/Stmts.hpp"

#include "TreeIO.hpp"

namespace sc::ast
{

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(Stmts stmt_type, ModuleLoc loc) : loc(loc), stype(stmt_type) {}
Stmt::~Stmt() {}

const char *Stmt::getStmtTypeCString() const
{
	switch(stype) {
	case BLOCK: return "block";
	case TYPE: return "type";
	case SIMPLE: return "simple";
	case EXPR: return "expression";
	case CALLARGS: return "function call args";
	case VAR: return "variable declaration base";
	case SIGNATURE: return "signature";
	case FNDEF: return "function definition";
	case VARDECL: return "variable declaration";
	case COND: return "conditional";
	case FOR: return "for loop";
	case ONEWORD: return "one word";
	}
	return "";
}

StringRef Stmt::getAttributeValue(StringRef name)
{
	auto loc = attrs.find(name);
	if(loc == attrs.end()) return "";
	return loc->second;
}

String Stmt::attributesToString(StringRef prefix, StringRef suffix)
{
	if(attrs.empty()) return "";
	String res(prefix);
	for(auto &a : attrs) {
		res += a.first;
		if(!a.second.empty()) {
			res += "=";
			res += a.second;
		}
		res += ", ";
	}
	res.pop_back();
	res.pop_back();
	res += suffix;
	return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtBlock ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtBlock::StmtBlock(ModuleLoc loc, const Vector<Stmt *> &stmts, bool is_top)
	: Stmt(BLOCK, loc), stmts(stmts), is_top(is_top)
{}
StmtBlock::~StmtBlock() {}
StmtBlock *StmtBlock::create(Allocator &allocator, ModuleLoc loc, const Vector<Stmt *> &stmts,
			     bool is_top)
{
	return allocator.alloc<StmtBlock>(loc, stmts, is_top);
}

void StmtBlock::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "Block [top = ", is_top ? "yes" : "no", "]",
		   attributesToString(" (", ")"), ":\n");
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!stmts[i]) {
			tio::tabAdd(has_next);
			tio::print(os, i != stmts.size() - 1, "<Source End>\n");
			tio::tabRem();
			continue;
		}
		stmts[i]->disp(os, i != stmts.size() - 1);
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtType /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtType::StmtType(ModuleLoc loc, Stmt *expr) : Stmt(TYPE, loc), expr(expr) {}
StmtType::~StmtType() {}
StmtType *StmtType::create(Allocator &allocator, ModuleLoc loc, Stmt *expr)
{
	return allocator.alloc<StmtType>(loc, expr);
}

void StmtType::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "Type Expr", attributesToString(" (", ")"), ":\n");
	expr->disp(os, false);
	tio::tabRem();
}

bool StmtType::isMetaType() const
{
	return expr && expr->getStmtType() == SIMPLE &&
	       as<StmtSimple>(expr)->getLexeme().isType(lex::IDEN) &&
	       as<StmtSimple>(expr)->getLexeme().getDataStr() == "type";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSimple::StmtSimple(ModuleLoc loc, const lex::Lexeme &val) : Stmt(SIMPLE, loc), val(val) {}

StmtSimple::~StmtSimple() {}
StmtSimple *StmtSimple::create(Allocator &allocator, ModuleLoc loc, const lex::Lexeme &val)
{
	return allocator.alloc<StmtSimple>(loc, val);
}

void StmtSimple::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "Simple", attributesToString(" (", ")"), ": ", val.str(0), "\n");
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// StmtFnCallInfo ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtCallArgs::StmtCallArgs(ModuleLoc loc, const Vector<Stmt *> &args)
	: Stmt(CALLARGS, loc), args(args)
{}
StmtCallArgs::~StmtCallArgs() {}
StmtCallArgs *StmtCallArgs::create(Allocator &allocator, ModuleLoc loc, const Vector<Stmt *> &args)
{
	return allocator.alloc<StmtCallArgs>(loc, args);
}

void StmtCallArgs::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "Function Call args", attributesToString(" (", ")"), ": ",
		   args.empty() ? "(empty)" : "", "\n");
	if(!args.empty()) {
		tio::tabAdd(false);
		tio::print(os, false, "Args:\n");
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(os, i != args.size() - 1);
		}
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExpr::StmtExpr(ModuleLoc loc, size_t commas, Stmt *lhs, const lex::Lexeme &oper, Stmt *rhs,
		   bool is_intrinsic_call)
	: Stmt(EXPR, loc), commas(commas), lhs(lhs), oper(oper), rhs(rhs), or_blk(nullptr),
	  or_blk_var(loc), is_intrinsic_call(is_intrinsic_call)
{}
StmtExpr::~StmtExpr() {}
StmtExpr *StmtExpr::create(Allocator &allocator, ModuleLoc loc, size_t commas, Stmt *lhs,
			   const lex::Lexeme &oper, Stmt *rhs, bool is_intrinsic_call)
{
	return allocator.alloc<StmtExpr>(loc, commas, lhs, oper, rhs, is_intrinsic_call);
}

void StmtExpr::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "Expression [parsing intinsic: ", is_intrinsic_call ? "yes" : "no",
		   "]", attributesToString(" (", ")"), ":\n");
	if(lhs) {
		tio::tabAdd(oper.isValid() || rhs || or_blk);
		tio::print(os, oper.isValid() || rhs || or_blk, "LHS:\n");
		lhs->disp(os, false);
		tio::tabRem();
	}
	if(oper.isValid()) {
		tio::tabAdd(rhs || or_blk);
		tio::print(os, rhs || or_blk, "Oper: ", oper.tokCStr(), "\n");
		tio::tabRem();
	}
	if(rhs) {
		tio::tabAdd(or_blk);
		tio::print(os, or_blk != nullptr, "RHS:\n");
		rhs->disp(os, false);
		tio::tabRem();
	}
	if(or_blk) {
		tio::tabAdd(false);
		StringRef orblkvardata =
		or_blk_var.isIdentifier() ? or_blk_var.getDataStr() : "<none>";
		tio::print(os, false, "Or: ", orblkvardata, "\n");
		or_blk->disp(os, false);
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVar::StmtVar(ModuleLoc loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval)
	: Stmt(VAR, loc), name(name), vtype(vtype), vval(vval)
{}
StmtVar::~StmtVar() {}
StmtVar *StmtVar::create(Allocator &allocator, ModuleLoc loc, const lex::Lexeme &name,
			 StmtType *vtype, Stmt *vval)
{
	return allocator.alloc<StmtVar>(loc, name, vtype, vval);
}

void StmtVar::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	// TODO: make sure the attribute name is correct
	tio::print(os, has_next, "Variable", attributesToString(" (", ")"), ": ", name.getDataStr(),
		   "\n");
	if(vtype) {
		tio::tabAdd(vval);
		tio::print(os, vval != nullptr, "Type:\n");
		vtype->disp(os, false);
		tio::tabRem();
	}
	if(vval) {
		tio::tabAdd(false);
		tio::print(os, false, "Value:\n");
		vval->disp(os, false);
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnSig ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSignature::StmtSignature(ModuleLoc loc, Vector<StmtVar *> &args, StmtType *rettype,
			     SignatureType sigty)
	: Stmt(SIGNATURE, loc), args(args), rettype(rettype), sigty(sigty)
{}
StmtSignature::~StmtSignature() {}
StmtSignature *StmtSignature::create(Allocator &allocator, ModuleLoc loc, Vector<StmtVar *> &args,
				     StmtType *rettype, SignatureType sigty)
{
	return allocator.alloc<StmtSignature>(loc, args, rettype, sigty);
}

void StmtSignature::disp(OStream &os, bool has_next)
{
	StringRef sigTyName = "";
	if(sigty == SignatureType::STRUCT) sigTyName = "Struct";
	else if(sigty == SignatureType::UNION) sigTyName = "Union";
	else if(sigty == SignatureType::FUNC) sigTyName = "Func";

	tio::tabAdd(has_next);
	tio::print(os, has_next, sigTyName, " Signature", attributesToString(" (", ")"), "\n");
	if(args.size() > 0) {
		tio::tabAdd(rettype);
		tio::print(os, rettype != nullptr, "Parameters:\n");
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(os, i != args.size() - 1);
		}
		tio::tabRem();
	}
	if(rettype) {
		tio::tabAdd(false);
		tio::print(os, false, "Return Type", "\n");
		rettype->disp(os, false);
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnDef::StmtFnDef(ModuleLoc loc, StmtSignature *sig, StmtBlock *blk)
	: Stmt(FNDEF, loc), sig(sig), blk(blk)
{}
StmtFnDef::~StmtFnDef() {}
StmtFnDef *StmtFnDef::create(Allocator &allocator, ModuleLoc loc, StmtSignature *sig,
			     StmtBlock *blk)
{
	return allocator.alloc<StmtFnDef>(loc, sig, blk);
}

void StmtFnDef::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "Function definition", attributesToString(" (", ")"), "\n");
	tio::tabAdd(true);
	tio::print(os, true, "Function Signature:\n");
	sig->disp(os, false);
	tio::tabRem();

	tio::tabAdd(false);
	tio::print(os, false, "Function Block:\n");
	blk->disp(os, false);
	tio::tabRem(2);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtVarDecl /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVarDecl::StmtVarDecl(ModuleLoc loc, const Vector<StmtVar *> &decls)
	: Stmt(VARDECL, loc), decls(decls)
{}
StmtVarDecl::~StmtVarDecl() {}
StmtVarDecl *StmtVarDecl::create(Allocator &allocator, ModuleLoc loc,
				 const Vector<StmtVar *> &decls)
{
	return allocator.alloc<StmtVarDecl>(loc, decls);
}

void StmtVarDecl::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "Variable declarations", attributesToString(" (", ")"), "\n");
	for(size_t i = 0; i < decls.size(); ++i) {
		decls[i]->disp(os, i != decls.size() - 1);
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Conditional::Conditional(Stmt *cond, StmtBlock *blk) : cond(cond), blk(blk) {}
Conditional::~Conditional() {}

StmtCond::StmtCond(ModuleLoc loc, Vector<Conditional> &&conds, bool is_inline)
	: Stmt(COND, loc), conds(std::move(conds)), is_inline(is_inline)
{}
StmtCond::StmtCond(ModuleLoc loc, const Vector<Conditional> &conds, bool is_inline)
	: Stmt(COND, loc), conds(conds), is_inline(is_inline)
{}
StmtCond::~StmtCond() {}
StmtCond *StmtCond::create(Allocator &allocator, ModuleLoc loc, Vector<Conditional> &&conds,
			   bool is_inline)
{
	return allocator.alloc<StmtCond>(loc, std::move(conds), is_inline);
}

void StmtCond::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "Conditional [is inline = ", is_inline ? "yes" : "no", "]",
		   attributesToString(" (", ")"), "\n");
	for(size_t i = 0; i < conds.size(); ++i) {
		tio::tabAdd(i != conds.size() - 1);
		tio::print(os, i != conds.size() - 1, "Branch:\n");
		if(conds[i].getCond()) {
			tio::tabAdd(true);
			tio::print(os, true, "Condition:\n");
			conds[i].getCond()->disp(os, false);
			tio::tabRem();
		}
		tio::tabAdd(false);
		tio::print(os, false, "Block:\n");
		conds[i].getBlk()->disp(os, false);
		tio::tabRem(2);
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFor //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFor::StmtFor(ModuleLoc loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk, bool is_inline)
	: Stmt(FOR, loc), init(init), cond(cond), incr(incr), blk(blk), is_inline(is_inline)
{}
StmtFor::~StmtFor() {}
StmtFor *StmtFor::create(Allocator &allocator, ModuleLoc loc, Stmt *init, Stmt *cond, Stmt *incr,
			 StmtBlock *blk, bool is_inline)
{
	return allocator.alloc<StmtFor>(loc, init, cond, incr, blk, is_inline);
}

void StmtFor::disp(OStream &os, bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(os, has_next, "For/While [is inline = ", is_inline ? "yes" : "no", "]",
		   attributesToString(" (", ")"), "\n");
	if(init) {
		tio::tabAdd(cond || incr || blk);
		tio::print(os, cond || incr || blk, "Init:\n");
		init->disp(os, false);
		tio::tabRem();
	}
	if(cond) {
		tio::tabAdd(incr || blk);
		tio::print(os, incr || blk, "Condition:\n");
		cond->disp(os, false);
		tio::tabRem();
	}
	if(incr) {
		tio::tabAdd(blk);
		tio::print(os, blk != nullptr, "Increment:\n");
		incr->disp(os, false);
		tio::tabRem();
	}
	if(blk) {
		tio::tabAdd(false);
		tio::print(os, false, "Block:\n");
		blk->disp(os, false);
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtOneWord::StmtOneWord(ModuleLoc loc, Stmt *arg, OneWordType wordty)
	: Stmt(ONEWORD, loc), arg(arg), wordty(wordty)
{}
StmtOneWord::~StmtOneWord() {}
StmtOneWord *StmtOneWord::create(Allocator &allocator, ModuleLoc loc, Stmt *arg, OneWordType wordty)
{
	return allocator.alloc<StmtOneWord>(loc, arg, wordty);
}

void StmtOneWord::disp(OStream &os, bool has_next)
{
	StringRef wordTyName = "";
	if(wordty == OneWordType::RETURN) wordTyName = "Return";
	else if(wordty == OneWordType::RETURN) wordTyName = "Return";
	else if(wordty == OneWordType::BREAK) wordTyName = "Break";
	else if(wordty == OneWordType::DEFER) wordTyName = "Defer";
	else if(wordty == OneWordType::GOTO) wordTyName = "Goto";

	tio::tabAdd(has_next);
	tio::print(os, has_next, wordTyName, attributesToString(" (", ")"), "\n");
	if(arg) {
		tio::tabAdd(false);
		tio::print(os, false, "Value:\n");
		arg->disp(os, false);
		tio::tabRem();
	}
	tio::tabRem();
}

} // namespace sc::ast