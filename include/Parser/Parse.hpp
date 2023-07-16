#pragma once

#include "ParseHelper.hpp"
#include "Stmts.hpp"

namespace sc
{
enum class Occurs
{
	YES,
	NO,
	MAYBE
};
class Parsing
{
	Context &ctx;

public:
	Parsing(Context &ctx);

	// on successful parse, returns true, and tree is allocated
	// if with_brace is true, it will attempt to find the beginning and ending brace for each
	// block
	bool parseBlock(ParseHelper &p, StmtBlock *&tree, bool with_brace = true);

	bool parseType(ParseHelper &p, StmtType *&type);
	bool parseSimple(ParseHelper &p, Stmt *&data);

	bool parsePrefixedSuffixedLiteral(ParseHelper &p, Stmt *&expr);
	bool parseExpr(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr17(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr16(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr15(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr14(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr13(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr12(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr11(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr10(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr09(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr08(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr07(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr06(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr05(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr04(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr03(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr02(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr01(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden);

	bool parseVar(ParseHelper &p, StmtVar *&var, const Occurs &intype, const Occurs &otype,
		      const Occurs &oval);

	bool parseFnSig(ParseHelper &p, Stmt *&fsig);
	bool parseFnDef(ParseHelper &p, Stmt *&fndef);

	bool parseHeader(ParseHelper &p, StmtHeader *&header);
	bool parseLib(ParseHelper &p, StmtLib *&lib);
	bool parseExtern(ParseHelper &p, Stmt *&ext);

	bool parseEnum(ParseHelper &p, Stmt *&ed);
	bool parseStruct(ParseHelper &p, Stmt *&sd, bool allowed_templs);

	bool parseVarDecl(ParseHelper &p, Stmt *&vd); // combines VAR_DECL_BASE and VAR_DECL

	bool parseConds(ParseHelper &p, Stmt *&conds);
	// this is just a transformation that generates a for loop
	bool parseForIn(ParseHelper &p, Stmt *&fin);
	bool parseFor(ParseHelper &p, Stmt *&f);
	bool parseWhile(ParseHelper &p, Stmt *&w);
	bool parseReturn(ParseHelper &p, Stmt *&ret);
	bool parseContinue(ParseHelper &p, Stmt *&cont);
	bool parseBreak(ParseHelper &p, Stmt *&brk);
	bool parseDefer(ParseHelper &p, Stmt *&defer);
};
} // namespace sc
