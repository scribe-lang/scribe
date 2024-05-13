#include "AST/Stmts.hpp"

#include "TreeIO.hpp"

namespace sc::AST
{

///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// Stmt //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

Stmt::Stmt(Stmts stmt_type, const ModuleLoc *loc)
	: loc(loc), derefcount(0), stype(stmt_type), stmtmask(0)
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
		res += ",";
	}
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

StmtType::StmtType(const ModuleLoc *loc, size_t ptr, bool variadic, Stmt *expr)
	: Stmt(TYPE, loc), ptr(ptr), variadic(variadic), expr(expr)
{}
StmtType::~StmtType() {}
StmtType *StmtType::create(Context &c, const ModuleLoc *loc, size_t ptr, bool variadic, Stmt *expr)
{
	return c.allocStmt<StmtType>(loc, ptr, variadic, expr);
}

void StmtType::disp(bool has_next)
{
	String tname(ptr, '*');
	if(variadic) tname = "..." + tname;

	if(!tname.empty()) {
		tio::tabAdd(has_next);
		tio::print(has_next || expr, "Type info", attributesToString(" (", ")"), ": ",
			   tname, "\n");
		tio::tabRem();
	}
	tio::tabAdd(has_next);
	tio::print(has_next, "Type Expr:\n");
	expr->disp(false);
	tio::tabRem();
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
	       as<StmtSimple>(expr)->getLexValue().isType(lex::TYPE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// StmtSimple /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtSimple::StmtSimple(const ModuleLoc *loc, const lex::Lexeme &val)
	: Stmt(SIMPLE, loc), decl(nullptr), val(val), self(nullptr),
	  disable_module_id_mangle(false), disable_codegen_mangle(false)
{}

StmtSimple::~StmtSimple() {}
StmtSimple *StmtSimple::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &val)
{
	return c.allocStmt<StmtSimple>(loc, val);
}

void StmtSimple::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Simple [decl = ", decl ? "yes" : "no",
		   "] [self = ", self ? "yes" : "no", "]", attributesToString(" (", ")"), ": ",
		   val.str(0), "\n");
	tio::tabRem();
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Function Call Info", attributesToString(" (", ")"), ": ",
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

StmtVar::StmtVar(const ModuleLoc *loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval,
		 uint8_t varmask)
	: Stmt(VAR, loc), name(name), vtype(vtype), vval(vval), varmask(varmask),
	  disable_module_id_mangle(false), disable_codegen_mangle(false)
{}
StmtVar::~StmtVar() {}
StmtVar *StmtVar::create(Context &c, const ModuleLoc *loc, const lex::Lexeme &name, StmtType *vtype,
			 Stmt *vval, uint8_t infomask)
{
	return c.allocStmt<StmtVar>(loc, name, vtype, vval, infomask);
}

void StmtVar::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Variable [in = ", isIn() ? "yes" : "no", "]",
		   attributesToString(" (", ")"), ": ", name.getDataStr(), "\n");
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Function signature [variadic = ", has_variadic ? "yes" : "no", "]",
		   attributesToString(" (", ")"), "\n");
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

StmtFnDef::StmtFnDef(const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk, bool is_inline)
	: Stmt(FNDEF, loc), sig(sig), blk(blk), parentvar(nullptr), used(0), is_inline(is_inline)
{}
StmtFnDef::~StmtFnDef() {}
StmtFnDef *StmtFnDef::create(Context &c, const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk,
			     bool is_inline)
{
	return c.allocStmt<StmtFnDef>(loc, sig, blk, is_inline);
}

void StmtFnDef::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Function definition [is inline: ", is_inline ? "yes" : "no",
		   "] [has parent: ", parentvar ? "yes" : "no", "]", attributesToString(" (", ")"),
		   "\n");
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Header\n");
	tio::tabAdd(!flags.getDataStr().empty());
	tio::print(!flags.getDataStr().empty(), "Names", attributesToString(" (", ")"), ": ",
		   names.getDataStr(), "\n");
	tio::tabRem();

	if(!flags.getDataStr().empty()) {
		tio::tabAdd(false);
		tio::print(false, "Flags: ", flags.getDataStr(), "\n");
		tio::tabRem();
	}
	tio::tabRem();
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Libs", attributesToString(" (", ")"), "\n");
	tio::tabAdd(false);
	tio::print(false, "Flags: ", flags.getDataStr(), "\n");
	tio::tabRem(2);
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Extern for ", fname.getDataStr(),
		   " [has parent: ", parentvar ? "yes" : "no", "]", attributesToString(" (", ")"),
		   "\n");
	if(headers) {
		tio::tabAdd(libs || entity);
		tio::print(libs || entity, "Headers:\n");
		headers->disp(false);
		tio::tabRem();
	}
	if(libs) {
		tio::tabAdd(entity);
		tio::print(entity != nullptr, "Libs:\n");
		libs->disp(false);
		tio::tabRem();
	}

	if(entity) {
		tio::tabAdd(false);
		tio::print(false, "Entity:\n");
		entity->disp(false);
		tio::tabRem();
	}
	tio::tabRem();
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Enumerations", attributesToString(" (", ")"), ":\n");
	if(tagty) {
		tio::tabAdd(!items.empty());
		tio::print(!items.empty(), "Provided Tag Type:\n");
		tagty->disp(false);
		tio::tabRem();
	}
	for(size_t i = 0; i < items.size(); ++i) {
		tio::tabAdd(i != items.size() - 1);
		tio::print(i != items.size() - 1, items[i].str(0), "\n");
		tio::tabRem();
	}
	tio::tabRem();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// StmtStruct //////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

StmtStruct::StmtStruct(const ModuleLoc *loc, const Vector<StmtVar *> &fields,
		       const Vector<lex::Lexeme> &templates, bool is_union, bool is_decl)
	: Stmt(STRUCTDEF, loc), fields(fields), templates(templates), is_union(is_union),
	  is_decl(is_decl), is_externed(false)
{}
StmtStruct::~StmtStruct() {}
StmtStruct *StmtStruct::create(Context &c, const ModuleLoc *loc, const Vector<StmtVar *> &fields,
			       const Vector<lex::Lexeme> &templates, bool is_union, bool is_decl)
{
	return c.allocStmt<StmtStruct>(loc, fields, templates, is_union, is_decl);
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

	tio::tabAdd(has_next);
	tio::print(has_next, is_union ? "Union" : "Struct", templatestr, is_decl ? " (decl)" : "",
		   attributesToString(" (", ")"), "\n");

	if(!fields.empty()) {
		tio::tabAdd(false);
		tio::print(false, "Fields:\n");
		for(size_t i = 0; i < fields.size(); ++i) {
			fields[i]->disp(i != fields.size() - 1);
		}
		tio::tabRem();
	}

	tio::tabRem();
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

StmtRet::StmtRet(const ModuleLoc *loc, Stmt *val) : Stmt(RET, loc), val(val) {}
StmtRet::~StmtRet() {}
StmtRet *StmtRet::create(Context &c, const ModuleLoc *loc, Stmt *val)
{
	return c.allocStmt<StmtRet>(loc, val);
}

void StmtRet::disp(bool has_next)
{
	tio::tabAdd(has_next);
	tio::print(has_next, "Return", attributesToString(" (", ")"), "\n");
	if(val) {
		tio::tabAdd(false);
		tio::print(false, "Value:\n");
		val->disp(false);
		tio::tabRem();
	}
	tio::tabRem();
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Continue", attributesToString(" (", ")"), "\n");
	tio::tabRem();
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
	tio::tabAdd(has_next);
	tio::print(has_next, "Break", attributesToString(" (", ")"), "\n");
	tio::tabRem();
}

} // namespace sc::AST