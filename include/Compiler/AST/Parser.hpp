#pragma once

#include "Allocator.hpp"
#include "DeferStack.hpp"
#include "ParseHelper.hpp"

namespace sc::ast
{

enum class Occurs
{
	YES,
	NO,
	MAYBE
};

class Parser
{
	ParseHelper p;
	DeferStack deferstack;
	Allocator &allocator;

public:
	Parser(Allocator &allocator, Vector<lex::Lexeme> &toks);

	// on successful parse, returns true, and tree is allocated
	// if with_brace is true, it will attempt to find the beginning and ending brace for each
	// block
	bool parseBlock(StmtBlock *&blk, bool with_brace = true);

	bool parseType(StmtType *&type);
	bool parseSimple(Stmt *&data);

	bool parsePrefixedSuffixedLiteral(Stmt *&expr);
	bool parseExpr(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr17(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr16(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr15(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr14(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr13(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr12(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr11(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr10(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr09(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr08(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr07(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr06(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr05(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr04(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr03(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr02(Stmt *&expr, bool disable_brace_after_iden);
	bool parseExpr01(Stmt *&expr, bool disable_brace_after_iden);

	bool parseVar(StmtVar *&var, const Occurs &intype, const Occurs &otype, const Occurs &oval);

	// for function signature, struct, and union
	bool parseSignature(Stmt *&sig);
	bool parseFnDef(Stmt *&fndef);

	bool parseVarDecl(Stmt *&vd);

	bool parseConds(Stmt *&conds);
	// this is just a transformation that generates a for loop
	bool parseForIn(Stmt *&fin);
	bool parseFor(Stmt *&f);
	bool parseWhile(Stmt *&w);
	// for return, continue, break, defer, and goto
	bool parseOneWord(Stmt *&word);

	bool parseAttributes(StringMap<String> &attrs);
};

// Can modify toks since some stuff requires it (like determining pre/post operators)
bool parse(Allocator &allocator, Vector<lex::Lexeme> &toks, StmtBlock *&tree);
void dumpTree(OStream &os, Stmt *tree);

} // namespace sc::ast
