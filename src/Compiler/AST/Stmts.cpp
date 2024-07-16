#include "AST/Stmts.hpp"

#include "TreeIO.hpp"

namespace sc::AST
{

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(Stmts stmt_type, const ModuleLoc *loc) : loc(loc), derefcount(0), stype(stmt_type) {}
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

StmtBlock::StmtBlock(const ModuleLoc *loc, const Vector<Stmt *> &stmts, bool is_top)
	: Stmt(BLOCK, loc), stmts(stmts), is_top(is_top)
{}
StmtBlock::~StmtBlock() {}
StmtBlock *StmtBlock::create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &stmts,
			     bool is_top)
{
	return c.allocStmt<StmtBlock>(loc, stmts, is_top);
}

void StmtBlock::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Block [top = ", is_top ? "yes" : "no", "]",
		   attributesToString(" (", ")"), ":\n");
	for(size_t i = 0; i < stmts.size(); ++i) {
		if(!stmts[i]) {
			tio::tabAdd(has_next);
			tio::print(i != stmts.size() - 1, "<Source End>\n");
			tio::tabRem();
			continue;
		}
		stmts[i]->disp(i != stmts.size() - 1);
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtType /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtType::StmtType(const ModuleLoc *loc, Stmt *expr) : Stmt(TYPE, loc), expr(expr) {}
StmtType::~StmtType() {}
StmtType *StmtType::create(Context &c, const ModuleLoc *loc, Stmt *expr)
{
	return c.allocStmt<StmtType>(loc, expr);
}

void StmtType::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Type Expr", attributesToString(" (", ")"), ":\n");
	expr->disp(false);
	tio::tabRem();
}

bool StmtType::isMetaType() const
{
	return expr && expr->getStmtType() == SIMPLE &&
	       as<StmtSimple>(expr)->getLexValue().isType(lex::TYPE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSimple::StmtSimple(const ModuleLoc *loc, const lex::Lexeme &val)
	: Stmt(SIMPLE, loc), decl(nullptr), val(val)
{}

StmtSimple::~StmtSimple() {}
StmtSimple *StmtSimple::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &val)
{
	return c.allocStmt<StmtSimple>(loc, val);
}

void StmtSimple::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Simple [decl = ", decl ? "yes" : "no", "]",
		   attributesToString(" (", ")"), ": ", val.str(0), "\n");
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// StmtFnCallInfo ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtCallArgs::StmtCallArgs(const ModuleLoc *loc, const Vector<Stmt *> &args)
	: Stmt(CALLARGS, loc), args(args)
{}
StmtCallArgs::~StmtCallArgs() {}
StmtCallArgs *StmtCallArgs::create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &args)
{
	return c.allocStmt<StmtCallArgs>(loc, args);
}

void StmtCallArgs::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Function Call args", attributesToString(" (", ")"), ": ",
		   args.empty() ? "(empty)" : "", "\n");
	if(!args.empty()) {
		tio::tabAdd(false);
		tio::print(false, "Args:\n");
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(i != args.size() - 1);
		}
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtExpr /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtExpr::StmtExpr(const ModuleLoc *loc, size_t commas, Stmt *lhs, const lex::Lexeme &oper,
		   Stmt *rhs, bool is_intrinsic_call)
	: Stmt(EXPR, loc), commas(commas), lhs(lhs), oper(oper), rhs(rhs), or_blk(nullptr),
	  or_blk_var(loc), is_intrinsic_call(is_intrinsic_call)
{}
StmtExpr::~StmtExpr() {}
StmtExpr *StmtExpr::create(Context &c, const ModuleLoc *loc, size_t commas, Stmt *lhs,
			   const lex::Lexeme &oper, Stmt *rhs, bool is_intrinsic_call)
{
	return c.allocStmt<StmtExpr>(loc, commas, lhs, oper, rhs, is_intrinsic_call);
}

void StmtExpr::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Expression [parsing intinsic: ", is_intrinsic_call ? "yes" : "no",
		   "]", attributesToString(" (", ")"), ":\n");
	if(lhs) {
		tio::tabAdd(oper.isValid() || rhs || or_blk);
		tio::print(oper.isValid() || rhs || or_blk, "LHS:\n");
		lhs->disp(false);
		tio::tabRem();
	}
	if(oper.isValid()) {
		tio::tabAdd(rhs || or_blk);
		tio::print(rhs || or_blk, "Oper: ", oper.tokCStr(), "\n");
		tio::tabRem();
	}
	if(rhs) {
		tio::tabAdd(or_blk);
		tio::print(or_blk != nullptr, "RHS:\n");
		rhs->disp(false);
		tio::tabRem();
	}
	if(or_blk) {
		tio::tabAdd(false);
		StringRef orblkvardata =
		or_blk_var.isIdentifier() ? or_blk_var.getDataStr() : "<none>";
		tio::print(false, "Or: ", orblkvardata, "\n");
		or_blk->disp(false);
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtVar //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtVar::StmtVar(const ModuleLoc *loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval)
	: Stmt(VAR, loc), name(name), vtype(vtype), vval(vval)
{}
StmtVar::~StmtVar() {}
StmtVar *StmtVar::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &name, StmtType *vtype,
			 Stmt *vval)
{
	return c.allocStmt<StmtVar>(loc, name, vtype, vval);
}

void StmtVar::disp(bool has_next)
{
	tio::tabAdd(has_next);
	// TODO: make sure the attribute name is correct
	tio::print(has_next, "Variable", attributesToString(" (", ")"), ": ", name.getDataStr(),
		   "\n");
	if(vtype) {
		tio::tabAdd(vval);
		tio::print(vval != nullptr, "Type:\n");
		vtype->disp(false);
		tio::tabRem();
	}
	if(vval) {
		tio::tabAdd(false);
		tio::print(false, "Value:\n");
		vval->disp(false);
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnSig ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSignature::StmtSignature(const ModuleLoc *loc, Vector<StmtVar *> &args, StmtType *rettype,
			     SignatureType sigty)
	: Stmt(SIGNATURE, loc), args(args), rettype(rettype), sigty(sigty)
{}
StmtSignature::~StmtSignature() {}
StmtSignature *StmtSignature::create(Context &c, const ModuleLoc *loc, Vector<StmtVar *> &args,
				     StmtType *rettype, SignatureType sigty)
{
	return c.allocStmt<StmtSignature>(loc, args, rettype, sigty);
}

void StmtSignature::disp(bool has_next)
{
	StringRef sigTyName = "";
	if(sigty == SignatureType::STRUCT) sigTyName = "Struct";
	else if(sigty == SignatureType::UNION) sigTyName = "Union";
	else if(sigty == SignatureType::FUNC) sigTyName = "Func";

	tio::tabAdd(has_next);
	tio::print(has_next, sigTyName, " Signature", attributesToString(" (", ")"), "\n");
	if(args.size() > 0) {
		tio::tabAdd(rettype);
		tio::print(rettype != nullptr, "Parameters:\n");
		for(size_t i = 0; i < args.size(); ++i) {
			args[i]->disp(i != args.size() - 1);
		}
		tio::tabRem();
	}
	if(rettype) {
		tio::tabAdd(false);
		tio::print(false, "Return Type", "\n");
		rettype->disp(false);
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtFnDef ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtFnDef::StmtFnDef(const ModuleLoc *loc, StmtSignature *sig, StmtBlock *blk)
	: Stmt(FNDEF, loc), sig(sig), blk(blk)
{}
StmtFnDef::~StmtFnDef() {}
StmtFnDef *StmtFnDef::create(Context &c, const ModuleLoc *loc, StmtSignature *sig, StmtBlock *blk)
{
	return c.allocStmt<StmtFnDef>(loc, sig, blk);
}

void StmtFnDef::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Function definition", attributesToString(" (", ")"), "\n");
	tio::tabAdd(true);
	tio::print(true, "Function Signature:\n");
	sig->disp(false);
	tio::tabRem();

	tio::tabAdd(false);
	tio::print(false, "Function Block:\n");
	blk->disp(false);
	tio::tabRem(2);
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Variable declarations", attributesToString(" (", ")"), "\n");
	for(size_t i = 0; i < decls.size(); ++i) {
		decls[i]->disp(i != decls.size() - 1);
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtCond /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Conditional::Conditional(Stmt *cond, StmtBlock *blk) : cond(cond), blk(blk) {}
Conditional::~Conditional() {}

StmtCond::StmtCond(const ModuleLoc *loc, Vector<Conditional> &&conds, bool is_inline)
	: Stmt(COND, loc), conds(std::move(conds)), is_inline(is_inline)
{}
StmtCond::StmtCond(const ModuleLoc *loc, const Vector<Conditional> &conds, bool is_inline)
	: Stmt(COND, loc), conds(conds), is_inline(is_inline)
{}
StmtCond::~StmtCond() {}
StmtCond *StmtCond::create(Context &c, const ModuleLoc *loc, Vector<Conditional> &&conds,
			   bool is_inline)
{
	return c.allocStmt<StmtCond>(loc, std::move(conds), is_inline);
}

void StmtCond::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Conditional [is inline = ", is_inline ? "yes" : "no", "]",
		   attributesToString(" (", ")"), "\n");
	for(size_t i = 0; i < conds.size(); ++i) {
		tio::tabAdd(i != conds.size() - 1);
		tio::print(i != conds.size() - 1, "Branch:\n");
		if(conds[i].getCond()) {
			tio::tabAdd(true);
			tio::print(true, "Condition:\n");
			conds[i].getCond()->disp(false);
			tio::tabRem();
		}
		tio::tabAdd(false);
		tio::print(false, "Block:\n");
		conds[i].getBlk()->disp(false);
		tio::tabRem(2);
	}
	tio::tabRem();
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
	tio::tabAdd(has_next);
	tio::print(has_next, "For/While [is inline = ", is_inline ? "yes" : "no", "]",
		   attributesToString(" (", ")"), "\n");
	if(init) {
		tio::tabAdd(cond || incr || blk);
		tio::print(cond || incr || blk, "Init:\n");
		init->disp(false);
		tio::tabRem();
	}
	if(cond) {
		tio::tabAdd(incr || blk);
		tio::print(incr || blk, "Condition:\n");
		cond->disp(false);
		tio::tabRem();
	}
	if(incr) {
		tio::tabAdd(blk);
		tio::print(blk != nullptr, "Increment:\n");
		incr->disp(false);
		tio::tabRem();
	}
	if(blk) {
		tio::tabAdd(false);
		tio::print(false, "Block:\n");
		blk->disp(false);
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// StmtRet //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtOneWord::StmtOneWord(const ModuleLoc *loc, Stmt *arg, OneWordType wordty)
	: Stmt(ONEWORD, loc), arg(arg), wordty(wordty)
{}
StmtOneWord::~StmtOneWord() {}
StmtOneWord *StmtOneWord::create(Context &c, const ModuleLoc *loc, Stmt *arg, OneWordType wordty)
{
	return c.allocStmt<StmtOneWord>(loc, arg, wordty);
}

void StmtOneWord::disp(bool has_next)
{
	StringRef wordTyName = "";
	if(wordty == OneWordType::RETURN) wordTyName = "Return";
	else if(wordty == OneWordType::RETURN) wordTyName = "Return";
	else if(wordty == OneWordType::BREAK) wordTyName = "Break";
	else if(wordty == OneWordType::DEFER) wordTyName = "Defer";
	else if(wordty == OneWordType::GOTO) wordTyName = "Goto";

	tio::tabAdd(has_next);
	tio::print(has_next, wordTyName, attributesToString(" (", ")"), "\n");
	if(arg) {
		tio::tabAdd(false);
		tio::print(false, "Value:\n");
		arg->disp(false);
		tio::tabRem();
	}
	tio::tabRem();
}

} // namespace sc::AST