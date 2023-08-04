#include "Parser/Parse.hpp"

#include <unordered_set>

#include "Error.hpp"

namespace sc
{
Parsing::Parsing(Context &ctx) : ctx(ctx) {}

// on successful parse, returns true, and tree is allocated
// if with_brace is true, it will attempt to find the beginning and ending brace for each block
bool Parsing::parseBlock(ParseHelper &p, StmtBlock *&tree, bool with_brace)
{
	tree = nullptr;

	Vector<Stmt *> stmts;
	Stmt *stmt = nullptr;

	lex::Lexeme &start = p.peek();

	if(with_brace) {
		if(!p.acceptn(lex::LBRACE)) {
			err::out(p.peek(), "expected opening braces '{' for block, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
	}

	while(p.isValid() && (!with_brace || !p.accept(lex::RBRACE))) {
		// TODO: this must be during simplify as all inline stuff is resolved as well
		// if(!with_brace && !p.accept(lex::LET)) {
		// 	err::out(p.peek(), "top level block can contain only 'let' declarations");
		// 	return false;
		// }
		bool skip_cols = false;
		// logic
		if(p.accept(lex::LET)) {
			if(!parseVarDecl(p, stmt)) return false;
		} else if(p.accept(lex::IF)) {
			if(!parseConds(p, stmt)) return false;
			skip_cols = true;
		} else if(p.accept(lex::INLINE)) {
			if(p.peekt(1) == lex::FOR) {
				if(!parseFor(p, stmt)) return false;
				skip_cols = true;
			} else if(p.peekt(1) == lex::WHILE) {
				if(!parseWhile(p, stmt)) return false;
				skip_cols = true;
			} else if(p.peekt(1) == lex::IF) {
				if(!parseConds(p, stmt)) return false;
				skip_cols = true;
			} else {
				err::out(p.peek(1), "'inline' is not applicable on '",
					 p.peek(1).getTok().cStr(), "' statement");
				return false;
			}
		} else if(p.accept(lex::FOR)) {
			if(p.peekt(1) == lex::IDEN && p.peekt(2) == lex::IN) {
				if(!parseForIn(p, stmt)) return false;
			} else {
				if(!parseFor(p, stmt)) return false;
			}
			skip_cols = true;
		} else if(p.accept(lex::WHILE)) {
			if(!parseWhile(p, stmt)) return false;
			skip_cols = true;
		} else if(p.accept(lex::RETURN)) {
			if(!parseReturn(p, stmt)) return false;
		} else if(p.accept(lex::CONTINUE)) {
			if(!parseContinue(p, stmt)) return false;
		} else if(p.accept(lex::BREAK)) {
			if(!parseBreak(p, stmt)) return false;
		} else if(p.accept(lex::DEFER)) {
			if(!parseDefer(p, stmt)) return false;
		} else if(p.accept(lex::LBRACE)) {
			if(!parseBlock(p, (StmtBlock *&)stmt)) return false;
			skip_cols = true;
		} else if(!parseExpr(p, stmt, false)) {
			return false;
		}

		if(skip_cols || p.acceptn(lex::COLS)) {
			stmts.push_back(stmt);
			stmt = nullptr;
			continue;
		}
		err::out(p.peek(), "expected semicolon for end of statement, found: ",
			 p.peek().getTok().cStr());
		return false;
	}

	if(with_brace) {
		if(!p.acceptn(lex::RBRACE)) {
			err::out(p.peek(), "expected closing braces '}' for block, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
	}

	tree = StmtBlock::create(ctx, start.getLoc(), stmts, !with_brace);
	return true;
}

bool Parsing::parseType(ParseHelper &p, StmtType *&type)
{
	type = nullptr;

	size_t ptr	 = 0;
	uint8_t stmtmask = 0;
	bool variadic	 = false;
	Stmt *count	 = nullptr;
	Stmt *expr	 = nullptr;
	bool dot_turn	 = false; // to ensure type name is in the form <name><dot><name>...

	lex::Lexeme &start = p.peek();

	if(p.accept(lex::FN)) {
		if(!parseFnSig(p, expr)) return false;
		type = StmtType::create(ctx, start.getLoc(), 0, false, expr);
		return true;
	}

	if(p.acceptn(lex::PreVA)) variadic = true;

	while(p.acceptn(lex::MUL)) ++ptr;

	if(p.acceptn(lex::BAND)) stmtmask |= (uint8_t)StmtMask::REF;
	if(p.acceptn(lex::CONST)) stmtmask |= (uint8_t)StmtMask::CONST;

	if(!parseExpr01(p, expr, true)) {
		err::out(p.peek(), "failed to parse type expression");
		return false;
	}

	if(!expr) {
		err::out(start, "no type expression found");
		return false;
	}

	type = StmtType::create(ctx, start.getLoc(), ptr, variadic, expr);
	type->setStmtMask(stmtmask);
	return true;
}

bool Parsing::parseSimple(ParseHelper &p, Stmt *&data)
{
	data = nullptr;

	if(!p.peek().getTok().isData()) {
		err::out(p.peek(), "expected data here, found: ", p.peek().getTok().cStr());
		return false;
	}

	lex::Lexeme &val = p.peek();
	p.next();

	data = StmtSimple::create(ctx, val.getLoc(), val);
	return true;
}

// ref"Ref of this"
// 9h
// 2.5i
bool Parsing::parsePrefixedSuffixedLiteral(ParseHelper &p, Stmt *&expr)
{
	lex::Lexeme &iden = p.peekt() == lex::IDEN ? p.peek() : p.peek(1);
	lex::Lexeme &lit  = p.peekt() == lex::IDEN ? p.peek(1) : p.peek();
	lex::Lexeme oper  = lex::Lexeme(iden.getLoc(), lex::TokType::FNCALL);

	p.next();
	p.next();

	StmtSimple *arg	      = StmtSimple::create(ctx, lit.getLoc(), lit);
	StmtSimple *fn	      = StmtSimple::create(ctx, iden.getLoc(), iden);
	StmtFnCallInfo *finfo = StmtFnCallInfo::create(ctx, arg->getLoc(), {arg});
	expr		      = StmtExpr::create(ctx, lit.getLoc(), 0, fn, oper, finfo, false);

	return true;
}

bool Parsing::parseExpr(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	return parseExpr17(p, expr, disable_brace_after_iden);
}

// Left Associative
// ,
bool Parsing::parseExpr17(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	size_t commas = 0;

	if(!parseExpr16(p, rhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::COMMA)) {
		++commas;
		oper = p.peek();
		p.next();
		if(!parseExpr16(p, lhs, disable_brace_after_iden)) {
			return false;
		}
		rhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		lhs = nullptr;
	}

	if(rhs->getStmtType() == EXPR) {
		as<StmtExpr>(rhs)->setCommas(commas);
	}

	expr = rhs;
	return true;
}
// Left Associative
// ?:
bool Parsing::parseExpr16(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs     = nullptr;
	Stmt *rhs     = nullptr;
	Stmt *lhs_lhs = nullptr;
	Stmt *lhs_rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr15(p, lhs, disable_brace_after_iden)) {
		return false;
	}
	if(!p.accept(lex::QUEST)) {
		expr = lhs;
		return true;
	}

	oper = p.peek();
	p.next();
	lex::Lexeme oper_inside;

	if(!parseExpr15(p, lhs_lhs, disable_brace_after_iden)) {
		return false;
	}
	if(!p.accept(lex::COL)) {
		err::out(p.peek(),
			 "expected ':' for ternary operator, found: ", p.peek().getTok().cStr());
		return false;
	}
	oper_inside = p.peek();
	p.next();
	if(!parseExpr15(p, lhs_rhs, disable_brace_after_iden)) {
		return false;
	}
	rhs = StmtExpr::create(ctx, oper.getLoc(), 0, lhs_lhs, oper_inside, lhs_rhs, false);
	goto after_quest;

after_quest:
	expr = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
	return true;
}
// Right Associative
// =
bool Parsing::parseExpr15(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr14(p, rhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ASSN)) {
		oper = p.peek();
		p.next();
		if(!parseExpr14(p, lhs, disable_brace_after_iden)) {
			return false;
		}
		rhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		lhs = nullptr;
	}

	expr = rhs;
	return true;
}
// Left Associative
// += -=
// *= /= %=
// <<= >>=
// &= |= ^=
// or-block
bool Parsing::parseExpr14(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs	  = nullptr;
	Stmt *rhs	  = nullptr;
	StmtBlock *or_blk = nullptr;
	lex::Lexeme or_blk_var;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr13(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ADD_ASSN, lex::SUB_ASSN, lex::MUL_ASSN) ||
	      p.accept(lex::DIV_ASSN, lex::MOD_ASSN, lex::LSHIFT_ASSN) ||
	      p.accept(lex::RSHIFT_ASSN, lex::BAND_ASSN, lex::BOR_ASSN) ||
	      p.accept(lex::BNOT_ASSN, lex::BXOR_ASSN))
	{
		oper = p.peek();
		p.next();
		if(!parseExpr13(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;

	if(!p.acceptn(lex::OR)) return true;

	if(p.accept(lex::IDEN)) {
		or_blk_var = p.peek();
		p.next();
	}

	if(!parseBlock(p, or_blk)) {
		return false;
	}
	if(expr->getStmtType() != EXPR) {
		expr = StmtExpr::create(ctx, expr->getLoc(), 0, expr, {}, nullptr, false);
	}
	as<StmtExpr>(expr)->setOr(or_blk, or_blk_var);
	return true;
}
// Left Associative
// ||
bool Parsing::parseExpr13(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr12(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr12(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// &&
bool Parsing::parseExpr12(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr11(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LAND)) {
		oper = p.peek();
		p.next();
		if(!parseExpr11(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// |
bool Parsing::parseExpr11(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr10(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr10(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// ^
bool Parsing::parseExpr10(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr09(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BXOR)) {
		oper = p.peek();
		p.next();
		if(!parseExpr09(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// &
bool Parsing::parseExpr09(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr08(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BAND)) {
		oper = p.peek();
		p.next();
		if(!parseExpr08(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// == !=
bool Parsing::parseExpr08(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr07(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::EQ, lex::NE)) {
		oper = p.peek();
		p.next();
		if(!parseExpr07(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// < <=
// > >=
bool Parsing::parseExpr07(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr06(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LT, lex::LE) || p.accept(lex::GT, lex::GE)) {
		oper = p.peek();
		p.next();
		if(!parseExpr06(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// << >>
bool Parsing::parseExpr06(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr05(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LSHIFT, lex::RSHIFT)) {
		oper = p.peek();
		p.next();
		if(!parseExpr05(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// + -
bool Parsing::parseExpr05(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr04(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ADD, lex::SUB)) {
		oper = p.peek();
		p.next();
		if(!parseExpr04(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Left Associative
// * / %
bool Parsing::parseExpr04(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peek();

	if(!parseExpr03(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::MUL, lex::DIV, lex::MOD)) {
		oper = p.peek();
		p.next();
		if(!parseExpr03(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
}
// Right Associative (single operand)
// ++ -- (pre)
// + - (unary)
// * & (deref, addrof)
// ! ~ (log/bit)
bool Parsing::parseExpr03(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;

	Vector<lex::Lexeme> opers;

	lex::Lexeme &start = p.peek();

	while(p.accept(lex::XINC, lex::XDEC) || p.accept(lex::ADD, lex::SUB) ||
	      p.accept(lex::MUL, lex::BAND) || p.accept(lex::LNOT, lex::BNOT))
	{
		if(p.peekt() == lex::XINC) p.sett(lex::INCX);
		if(p.peekt() == lex::XDEC) p.sett(lex::DECX);
		if(p.peekt() == lex::ADD) p.sett(lex::UADD);
		if(p.peekt() == lex::SUB) p.sett(lex::USUB);
		if(p.peekt() == lex::MUL) p.sett(lex::UMUL);
		if(p.peekt() == lex::BAND) p.sett(lex::UAND);
		opers.insert(opers.begin(), p.peek());
		p.next();
	}

	if(!parseExpr02(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	if(!lhs) {
		err::out(start, "invalid expression");
		return false;
	}

	if(lhs->isSimple() && !opers.empty()) {
		lex::Lexeme &val = as<StmtSimple>(lhs)->getLexValue();
		lex::TokType tk	 = val.getTokVal();
		if(tk == lex::INT) {
			while(!opers.empty() && opers.front().getTokVal() == lex::USUB) {
				val.setDataInt(-val.getDataInt());
				opers.erase(opers.begin());
			}
		}
		if(tk == lex::FLT) {
			while(!opers.empty() && opers.front().getTokVal() == lex::USUB) {
				val.setDataFlt(-val.getDataFlt());
				opers.erase(opers.begin());
			}
		}
	}

	for(auto &op : opers) {
		lhs = StmtExpr::create(ctx, op.getLoc(), 0, lhs, op, nullptr, false);
	}

	expr = lhs;
	return true;
}
// Left Associative
// ++ -- (post)
// ... (postva)
bool Parsing::parseExpr02(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;

	Vector<lex::Lexeme> opers;

	lex::Lexeme &start = p.peek();

	if(!parseExpr01(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	if(p.accept(lex::XINC, lex::XDEC, lex::PreVA)) {
		if(p.peekt() == lex::PreVA) p.sett(lex::PostVA);
		lhs = StmtExpr::create(ctx, p.peek().getLoc(), 0, lhs, p.peek(), nullptr, false);
		p.next();
	}

	expr = lhs;
	return true;
}
bool Parsing::parseExpr01(ParseHelper &p, Stmt *&expr, bool disable_brace_after_iden)
{
	expr = nullptr;

	lex::Lexeme &start = p.peek();
	lex::Lexeme dot;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;
	Vector<Stmt *> args;
	Stmt *arg	  = nullptr;
	bool is_intrinsic = false;

	// prefixed/suffixed literals
	if(p.accept(lex::IDEN) && p.peek(1).getTok().isLiteral() ||
	   p.peek().getTok().isLiteral() && p.peekt(1) == lex::IDEN)
	{
		return parsePrefixedSuffixedLiteral(p, expr);
	}

	if(p.acceptn(lex::LPAREN)) {
		if(!parseExpr(p, lhs, disable_brace_after_iden)) {
			return false;
		}
		if(!p.acceptn(lex::RPAREN)) {
			err::out(p.peek(),
				 "expected ending parenthesis ')' for expression, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
	}

begin:
	if(p.acceptn(lex::AT)) is_intrinsic = true;
	if(p.acceptd() && !parseSimple(p, lhs)) return false;
	goto begin_brack;

after_dot:
	if(!p.acceptd() || !parseSimple(p, rhs)) return false;
	if(lhs && rhs) {
		lhs = StmtExpr::create(ctx, dot.getLoc(), 0, lhs, dot, rhs, false);
		rhs = nullptr;
	}

begin_brack:
	if(p.accept(lex::LBRACK)) {
		lex::Lexeme oper;
		p.sett(lex::SUBS);
		oper = p.peek();
		p.next();
		if(is_intrinsic) {
			err::out(p.peek(), "only function calls can be intrinsic;"
					   " attempted subscript here");
			return false;
		}
		if(!parseExpr16(p, rhs, false)) {
			err::out(oper, "failed to parse expression for subscript");
			return false;
		}
		if(!p.acceptn(lex::RBRACK)) {
			err::out(p.peek(),
				 "expected closing bracket for"
				 " subscript expression, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
		lhs = StmtExpr::create(ctx, oper.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
		if(p.accept(lex::LBRACK, lex::LPAREN) ||
		   (p.peekt() == lex::DOT && p.peekt(1) == lex::LT))
			goto begin_brack;
	} else if(p.accept(lex::LPAREN) || (!disable_brace_after_iden && p.accept(lex::LBRACE))) {
		bool fncall = p.accept(lex::LPAREN);
		lex::Lexeme oper;
		if(!p.accept(lex::LPAREN, lex::LBRACE)) {
			err::out(p.peek(),
				 "expected opening parenthesis/brace"
				 " for function/struct call, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
		p.sett(fncall ? lex::FNCALL : lex::STCALL);
		oper = p.peek();
		p.next();
		if(p.acceptn(fncall ? lex::RPAREN : lex::RBRACE)) {
			goto post_args;
		}
		// parse arguments
		while(true) {
			if(!parseExpr16(p, arg, false)) return false;
			args.push_back(arg);
			arg = nullptr;
			if(!p.acceptn(lex::COMMA)) break;
		}
		if(!p.acceptn(fncall ? lex::RPAREN : lex::RBRACE)) {
			err::out(p.peek(),
				 "expected closing parenthesis/brace after "
				 "function/struct call arguments, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
	post_args:
		rhs  = StmtFnCallInfo::create(ctx, oper.getLoc(), args);
		lhs  = StmtExpr::create(ctx, oper.getLoc(), 0, lhs, oper, rhs, is_intrinsic);
		rhs  = nullptr;
		args = {};

		if(!disable_brace_after_iden && p.accept(lex::LBRACE)) goto begin_brack;
		if(p.accept(lex::LBRACK, lex::LPAREN)) goto begin_brack;
	}

dot:
	if(p.acceptn(lex::DOT, lex::ARROW)) {
		if(lhs && rhs) {
			lhs =
			StmtExpr::create(ctx, p.peek(-1).getLoc(), 0, lhs, p.peek(-1), rhs, false);
			rhs = nullptr;
		}
		dot = p.peek(-1);
		goto after_dot;
	}

done:
	if(lhs && rhs) {
		lhs = StmtExpr::create(ctx, dot.getLoc(), 0, lhs, dot, rhs, false);
		rhs = nullptr;
	}
	expr = lhs;
	return true;
}

bool Parsing::parseVar(ParseHelper &p, StmtVar *&var, const Occurs &intype, const Occurs &otype,
		       const Occurs &oval)
{
	var = nullptr;

	uint8_t stmtmask = 0;
	uint8_t varmask	 = 0;
	while(p.accept(lex::STATIC, lex::VOLATILE) || p.accept(lex::GLOBAL, lex::COMPTIME)) {
		if(p.acceptn(lex::COMPTIME)) stmtmask |= (uint8_t)StmtMask::COMPTIME;
		if(p.acceptn(lex::STATIC)) varmask |= (uint8_t)VarMask::STATIC;
		if(p.acceptn(lex::VOLATILE)) varmask |= (uint8_t)VarMask::VOLATILE;
		if(p.acceptn(lex::GLOBAL)) varmask |= (uint8_t)VarMask::GLOBAL;
	}

	bool comptime = stmtmask & (uint8_t)StmtMask::COMPTIME;
	bool stati    = varmask & (uint8_t)VarMask::STATIC;
	bool volatil  = varmask & (uint8_t)VarMask::VOLATILE;
	bool global   = varmask & (uint8_t)VarMask::GLOBAL;

	if(!p.accept(lex::IDEN)) {
		err::out(p.peek(), "expected identifier for variable name, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	lex::Lexeme &name = p.peek();
	p.next();
	StmtType *in   = nullptr;
	StmtType *type = nullptr;
	Stmt *val      = nullptr;

in:
	if(intype == Occurs::NO && p.accept(lex::IN)) {
		err::out(p.peek(), "unexpected 'in' here");
		return false;
	}
	if(!p.acceptn(lex::IN)) {
		goto type;
	}
	if(stati || volatil || global || comptime) {
		err::out(p.peek(), "static, volatile, global, and comptime",
			 " can be used only for data variables");
		return false;
	}
	if(!parseType(p, in)) {
		err::out(p.peek(), "failed to parse in-type for variable: ", name.getDataStr());
		return false;
	}
	varmask |= (uint8_t)VarMask::IN;

type:
	if(otype == Occurs::NO && p.accept(lex::COL)) {
		err::out(p.peek(), "unexpected beginning of type here");
		return false;
	}
	if(!p.acceptn(lex::COL)) {
		goto val;
	}
	if(!parseType(p, type)) {
		err::out(p.peek(), "failed to parse type for variable: ", name.getDataStr());
		return false;
	}
	if(type->isMetaType() && !comptime) {
		err::out(type, "a variable of type 'type' must be comptime");
		return false;
	}

val:
	if(oval == Occurs::NO && p.accept(lex::ASSN)) {
		err::out(p.peek(), "unexpected beginning of value assignment here");
		return false;
	}
	if(!p.acceptn(lex::ASSN)) {
		goto done;
	}
	if(comptime && (p.accept(lex::ENUM, lex::STRUCT) || p.accept(lex::FN, lex::EXTERN))) {
		err::out(p.peek(), "comptime declaration can only have an expression as value");
		return false;
	}
	if(p.accept(lex::ENUM)) {
		if(!parseEnum(p, val)) return false;
	} else if(p.accept(lex::STRUCT)) {
		if(!parseStruct(p, val, true)) return false;
	} else if(p.accept(lex::INLINE, lex::FN)) {
		if(!parseFnDef(p, val)) return false;
	} else if(p.accept(lex::EXTERN)) {
		if(!parseExtern(p, val)) return false;
		if(!as<StmtExtern>(val)->getEntity() && !type) {
			err::out(name, "variable extern must have a type");
			return false;
		}
	} else if(!parseExpr16(p, val, false)) {
		return false;
	}

done:
	if(!type && !val) {
		err::out(name, "invalid variable declaration - no type or value set");
		return false;
	}
	if(comptime && !val && oval != Occurs::NO) {
		err::out(name, "comptime variable cannot be declared without an expression");
		return false;
	}
	if(in) {
		if(type) {
			err::out(name, "let-in statements can only have values "
				       "(function definitions) - no types allowed");
			return false;
		}
		if(val && val->getStmtType() != FNDEF) {
			err::out(name, "only functions can be created using let-in statements");
			return false;
		}
		lex::Lexeme selfeme(in->getLoc(), lex::IDEN, "self");
		in->setRef();
		StmtVar *selfvar  = StmtVar::create(ctx, in->getLoc(), selfeme, in, nullptr, 0);
		StmtFnSig *valsig = as<StmtFnDef>(val)->getSig();
		valsig->getArgs().insert(valsig->getArgs().begin(), selfvar);
	}
	var = StmtVar::create(ctx, name.getLoc(), name, type, val, varmask);
	var->setStmtMask(stmtmask);
	return true;
}

bool Parsing::parseFnSig(ParseHelper &p, Stmt *&fsig)
{
	fsig = nullptr;

	Vector<StmtVar *> args;
	StmtVar *var = nullptr;
	Set<StringRef> argnames;
	StmtType *rettype  = nullptr;
	bool found_va	   = false;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::FN)) {
		err::out(p.peek(), "expected 'fn' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!p.acceptn(lex::LPAREN)) {
		err::out(p.peek(), "expected opening parenthesis for function args, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	if(p.acceptn(lex::RPAREN)) goto post_args;

	// args
	while(true) {
		bool is_comptime = false;
		if(p.acceptn(lex::COMPTIME)) is_comptime = true;
		if(argnames.find(p.peek().getDataStr()) != argnames.end()) {
			err::out(p.peek(), "this argument name is already used "
					   "before in this function signature");
			return false;
		}
		argnames.insert(p.peek().getDataStr());
		if(is_comptime) p.setPos(p.getPos() - 1);
		if(!parseVar(p, var, Occurs::NO, Occurs::YES, Occurs::NO)) {
			return false;
		}
		if(var->getVType()->isVariadic()) {
			found_va = true;
		}
		Stmt *vtexpr = var->getVType()->getExpr();
		args.push_back(var);
		var = nullptr;
		if(!p.acceptn(lex::COMMA)) break;
		if(found_va) {
			err::out(p.peek(), "no parameter can exist after variadic");
			return false;
		}
	}

	if(!p.acceptn(lex::RPAREN)) {
		err::out(p.peek(), "expected closing parenthesis after function args, found: ",
			 p.peek().getTok().cStr());
		return false;
	}

post_args:
	if(p.acceptn(lex::COL) && !parseType(p, rettype)) {
		err::out(p.peek(), "failed to parse return type for function");
		return false;
	}
	if(!rettype) {
		lex::Lexeme voideme = lex::Lexeme(p.peek(-1).getLoc(), lex::VOID, "void");
		StmtSimple *voidsim = StmtSimple::create(ctx, voideme.getLoc(), voideme);
		rettype		    = StmtType::create(ctx, voidsim->getLoc(), 0, false, voidsim);
	}

	fsig = StmtFnSig::create(ctx, start.getLoc(), args, rettype, found_va);
	return true;
}
bool Parsing::parseFnDef(ParseHelper &p, Stmt *&fndef)
{
	fndef = nullptr;

	Stmt *sig	   = nullptr;
	StmtBlock *blk	   = nullptr;
	bool is_inline	   = false;
	lex::Lexeme &start = p.peek();

	if(p.acceptn(lex::INLINE)) is_inline = true;

	if(!parseFnSig(p, sig)) return false;
	if(!parseBlock(p, blk)) return false;

	fndef = StmtFnDef::create(ctx, start.getLoc(), (StmtFnSig *)sig, blk, is_inline);
	return true;
}

bool Parsing::parseHeader(ParseHelper &p, StmtHeader *&header)
{
	header = nullptr;

	lex::Lexeme names, flags;

	if(!p.accept(lex::IDEN, lex::STR)) {
		err::out(p.peek(), "expected string or identifier for the name of header, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	names = p.peek();
	p.next();

	if(!p.acceptn(lex::COL)) goto done;

	if(!p.accept(lex::IDEN, lex::STR)) {
		err::out(p.peek(), "expected string or identifier for the header flags, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	flags = p.peek();
	p.next();

done:
	header = StmtHeader::create(ctx, names.getLoc(), names, flags);
	return true;
}
bool Parsing::parseLib(ParseHelper &p, StmtLib *&lib)
{
	lib = nullptr;

	lex::Lexeme flags;

	if(!p.accept(lex::IDEN, lex::STR)) {
		err::out(p.peek(), "expected string or identifier for the lib flags, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	flags = p.peek();
	p.next();

	lib = StmtLib::create(ctx, flags.getLoc(), flags);
	return true;
}
bool Parsing::parseExtern(ParseHelper &p, Stmt *&ext)
{
	ext = nullptr;

	lex::Lexeme name; // name of the function
	StmtHeader *headers = nullptr;
	StmtLib *libs	    = nullptr;
	Stmt *entity	    = nullptr;
	bool struct_kw	    = false;

	if(!p.acceptn(lex::EXTERN)) {
		err::out(p.peek(),
			 "expected extern keyword here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!p.acceptn(lex::LBRACK)) {
		err::out(p.peek(), "expected opening bracket for extern information, found: ",
			 p.peek().getTok().cStr());
		return false;
	}

	if(p.acceptn(lex::STRUCT)) struct_kw = true;
	if(!p.accept(lex::IDEN)) {
		err::out(p.peek(),
			 "expected identifier or string here, found: ", p.peek().getTok().cStr());
		return false;
	}
	name = p.peek();
	p.next();

	// This is for handling typedefs and structs in C
	// as 'struct' needs to be prepended for structs
	if(struct_kw) name.setDataStr(ctx.strFrom({"struct ", name.getDataStr()}));

	if(!p.acceptn(lex::COMMA)) goto endinfo;

	if(!parseHeader(p, headers)) return false;
	if(!p.acceptn(lex::COMMA)) goto endinfo;
	if(!parseLib(p, libs)) return false;

endinfo:
	if(!p.acceptn(lex::RBRACK)) {
		err::out(p.peek(), "expected closing bracket after extern information, found: ",
			 p.peek().getTok().cStr());
		return false;
	}

	if(p.accept(lex::FN)) {
		if(!parseFnSig(p, entity)) return false;
	} else if(p.accept(lex::STRUCT)) {
		if(!parseStruct(p, entity, false)) return false;
	}

end:
	ext = StmtExtern::create(ctx, name.getLoc(), name, headers, libs, entity);
	return true;
}

bool Parsing::parseEnum(ParseHelper &p, Stmt *&ed)
{
	Vector<lex::Lexeme> enumvars;
	StmtType *tagty = nullptr;

	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::ENUM)) {
		err::out(p.peek(),
			 "expected 'enum' keyword here, found: ", p.peek().getTok().cStr());
		return false;
	}
	if(p.acceptn(lex::COL) && !parseType(p, tagty)) {
		err::out(p.peek(), "expected tag type for enum, found: ", p.peek().getTok().cStr());
		return false;
	}
	if(!p.acceptn(lex::LBRACE)) {
		err::out(p.peek(), "expected left braces for beginning of enum list, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	while(p.accept(lex::IDEN)) {
		enumvars.push_back(p.peek());
		p.next();
		if(!p.acceptn(lex::COMMA)) break;
	}
	if(!p.acceptn(lex::RBRACE)) {
		err::out(p.peek(), "expected right braces for end of enum list, found: ",
			 p.peek().getTok().cStr());
		return false;
	}
	if(enumvars.empty()) {
		err::out(start, "cannot have empty enumeration");
		return false;
	}
	ed = StmtEnum::create(ctx, start.getLoc(), enumvars, tagty);
	return true;
}

bool Parsing::parseStruct(ParseHelper &p, Stmt *&sd, bool allowed_templs)
{
	sd = nullptr;

	Vector<StmtVar *> fields;
	Vector<lex::Lexeme> templates;
	StmtVar *field = nullptr;
	Set<StringRef> fieldnames;
	bool is_decl	   = false;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::STRUCT)) {
		err::out(p.peek(),
			 "expected 'struct' keyword here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(p.acceptn(lex::LT)) {
		if(!allowed_templs) {
			err::out(p.peek(), "templates are not allowed in externed structs");
			return false;
		}
		while(p.accept(lex::IDEN)) {
			templates.push_back(p.peek());
			p.next();
			if(!p.acceptn(lex::COMMA)) break;
		}
		if(!p.acceptn(lex::GT)) {
			err::out(p.peek(), "expected '>' for end of struct template list, found: ",
				 p.peek().getTok().cStr());
			return false;
		}
	}

	if(!p.acceptn(lex::LBRACE)) {
		is_decl = true;
		goto done;
	}

	while(p.accept(lex::IDEN, lex::COMPTIME)) {
		if(!parseVar(p, field, Occurs::NO, Occurs::YES, Occurs::NO)) return false;
		if(fieldnames.find(field->getName().getDataStr()) != fieldnames.end()) {
			err::out(p.peek(), "this field name is already used "
					   "before in this same structure");
			return false;
		}
		fieldnames.insert(field->getName().getDataStr());
		fields.push_back(field);
		field = nullptr;
		if(!p.acceptn(lex::COLS)) break;
	}
	if(!p.acceptn(lex::RBRACE)) {
		err::out(p.peek(),
			 "expected closing brace for struct/trait"
			 " declaration/definition, found: ",
			 p.peek().getTok().cStr());
		return false;
	}

done:
	sd = StmtStruct::create(ctx, start.getLoc(), fields, templates, is_decl);
	return true;
}

bool Parsing::parseVarDecl(ParseHelper &p, Stmt *&vd)
{
	vd = nullptr;

	Vector<StmtVar *> decls;
	StmtVar *decl	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::LET)) {
		err::out(p.peek(),
			 "expected 'let' keyword here, found: ", p.peek().getTok().cStr());
		return false;
	}

	while(p.accept(lex::IDEN, lex::COMPTIME, lex::GLOBAL) ||
	      p.accept(lex::CONST, lex::STATIC, lex::VOLATILE))
	{
		if(!parseVar(p, decl, Occurs::MAYBE, Occurs::MAYBE, Occurs::MAYBE)) return false;
		decls.push_back(decl);
		decl = nullptr;
		if(!p.acceptn(lex::COMMA)) break;
	}

	vd = StmtVarDecl::create(ctx, start.getLoc(), decls);
	return true;
}

bool Parsing::parseConds(ParseHelper &p, Stmt *&conds)
{
	conds = nullptr;

	Vector<Conditional> cvec;
	Conditional c(nullptr, nullptr);
	bool is_inline = false;

	if(p.acceptn(lex::INLINE)) is_inline = true;

	lex::Lexeme &start = p.peek();

cond:
	if(!p.acceptn(lex::IF, lex::ELIF)) {
		err::out(p.peek(), "expected 'if' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseExpr15(p, c.getCond(), true)) {
		err::out(p.peek(), "failed to parse condition for if/else if statement");
		return false;
	}

blk:
	if(!parseBlock(p, c.getBlk())) {
		err::out(p.peek(), "failed to parse block for conditional");
		return false;
	}

	cvec.emplace_back(c.getCond(), c.getBlk());
	c.reset();

	if(p.accept(lex::ELIF)) goto cond;
	if(p.acceptn(lex::ELSE)) goto blk;

	conds = StmtCond::create(ctx, start.getLoc(), cvec, is_inline);
	return true;
}
// For-In transformation:
//
// for e in vec.eachRev() {
// 	...
// }
// ----------------------
// will generate
// ----------------------
// {
// let e_interm = vec.eachRev();
// for let _e = e_interm.begin(); _e != e_interm.end(); _e = e_interm.next(_e) {
// 	let e = e_interm.at(_e); // e is a reference
// 	...
// }
// }
bool Parsing::parseForIn(ParseHelper &p, Stmt *&fin)
{
	fin = nullptr;

	lex::Lexeme iter;
	Stmt *in	   = nullptr; // L01
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::FOR)) {
		err::out(p.peek(), "expected 'for' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!p.accept(lex::IDEN)) {
		err::out(p.peek(),
			 "expected iterator (identifier) here, found: ", p.peek().getTok().cStr());
		return false;
	}
	iter = p.peek();
	p.next();

	if(!p.acceptn(lex::IN)) {
		err::out(p.peek(), "expected 'in' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseExpr01(p, in, true)) {
		err::out(p.peek(), "failed to parse expression for 'in'");
		return false;
	}

	if(!p.accept(lex::LBRACE)) {
		err::out(p.peek(),
			 "expected block for for-in construct, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseBlock(p, blk)) {
		err::out(p.peek(), "failed to parse block for for-in construct");
		return false;
	}

	const ModuleLoc *loc	= iter.getLoc();
	lex::Lexeme in_interm	= iter; // e_interm
	lex::Lexeme iter_interm = iter; // _e
	lex::Lexeme lexbegin	= lex::Lexeme(loc, lex::IDEN, "begin");
	lex::Lexeme lexend	= lex::Lexeme(loc, lex::IDEN, "end");
	lex::Lexeme lexnext	= lex::Lexeme(loc, lex::IDEN, "next");
	lex::Lexeme lexat	= lex::Lexeme(loc, lex::IDEN, "at");
	lex::Lexeme dot_op	= lex::Lexeme(loc, lex::DOT);
	lex::Lexeme call_op	= lex::Lexeme(loc, lex::FNCALL);
	lex::Lexeme ne_op	= lex::Lexeme(loc, lex::NE);
	lex::Lexeme assn_op	= lex::Lexeme(loc, lex::ASSN);

	in_interm.setDataStr(ctx.strFrom({in_interm.getDataStr(), "_interm"}));
	iter_interm.setDataStr(ctx.strFrom({"_", iter_interm.getDataStr()}));

	StmtVar *in_interm_var =
	StmtVar::create(ctx, in_interm.getLoc(), in_interm, nullptr, in, 0);
	// block statement 1:
	StmtVarDecl *in_interm_vardecl = StmtVarDecl::create(ctx, loc, {in_interm_var});

	// init:
	// let <iter_interm> = <in_interm>.begin()
	StmtSimple *init_lhs	= StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *init_rhs	= StmtSimple::create(ctx, loc, lexbegin);
	StmtExpr *init_dot_expr = StmtExpr::create(ctx, loc, 0, init_lhs, dot_op, init_rhs, false);
	StmtFnCallInfo *init_call_info = StmtFnCallInfo::create(ctx, loc, {});
	StmtExpr *init_expr =
	StmtExpr::create(ctx, loc, 0, init_dot_expr, call_op, init_call_info, false);
	StmtVar *init_iter_interm_var =
	StmtVar::create(ctx, iter_interm.getLoc(), iter_interm, nullptr, init_expr, 0);
	StmtVarDecl *init = StmtVarDecl::create(ctx, iter_interm.getLoc(), {init_iter_interm_var});

	// cond:
	// <iter_interm> != <in_interm>.end()
	StmtSimple *cond_rhs_lhs = StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *cond_rhs_rhs = StmtSimple::create(ctx, loc, lexend);
	StmtExpr *cond_dot_expr =
	StmtExpr::create(ctx, loc, 0, cond_rhs_lhs, dot_op, cond_rhs_rhs, false);
	StmtFnCallInfo *cond_call_info = StmtFnCallInfo::create(ctx, loc, {});
	StmtExpr *cond_rhs =
	StmtExpr::create(ctx, loc, 0, cond_dot_expr, call_op, cond_call_info, false);
	StmtSimple *cond_lhs = StmtSimple::create(ctx, iter_interm.getLoc(), iter_interm);
	StmtExpr *cond	     = StmtExpr::create(ctx, loc, 0, cond_lhs, ne_op, cond_rhs, false);

	// incr:
	// <iter_interm> = <in_interm>.next(<iter_interm>)
	StmtSimple *incr_lhs_lhs = StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *incr_lhs_rhs = StmtSimple::create(ctx, loc, lexnext);
	StmtExpr *incr_dot_expr =
	StmtExpr::create(ctx, loc, 0, incr_lhs_lhs, dot_op, incr_lhs_rhs, false);
	StmtSimple *incr_call_arg      = StmtSimple::create(ctx, loc, iter_interm);
	StmtFnCallInfo *incr_call_info = StmtFnCallInfo::create(ctx, loc, {incr_call_arg});
	StmtExpr *incr_lhs =
	StmtExpr::create(ctx, loc, 0, incr_dot_expr, call_op, incr_call_info, false);
	StmtSimple *incr_rhs = StmtSimple::create(ctx, iter_interm.getLoc(), iter_interm);
	StmtExpr *incr	     = StmtExpr::create(ctx, loc, 0, incr_lhs, assn_op, incr_rhs, false);

	// inside loop block:
	// let <iter> = <in_interm>.at(<iter_interm>)
	StmtSimple *loop_var_val_lhs = StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *loop_var_val_rhs = StmtSimple::create(ctx, loc, lexat);
	StmtExpr *loop_var_val_dot_expr =
	StmtExpr::create(ctx, loc, 0, loop_var_val_lhs, dot_op, loop_var_val_rhs, false);
	StmtSimple *loop_var_val_call_arg = StmtSimple::create(ctx, loc, iter_interm);
	StmtFnCallInfo *loop_var_val_call_info =
	StmtFnCallInfo::create(ctx, loc, {loop_var_val_call_arg});
	StmtExpr *loop_var_val	  = StmtExpr::create(ctx, loc, 0, loop_var_val_dot_expr, call_op,
						     loop_var_val_call_info, false);
	StmtVar *loop_var	  = StmtVar::create(ctx, loc, iter, nullptr, loop_var_val, 0);
	StmtVarDecl *loop_vardecl = StmtVarDecl::create(ctx, loc, {loop_var});
	blk->getStmts().insert(blk->getStmts().begin(), loop_vardecl);

	// StmtFor - block statement 2:
	StmtFor *loop = StmtFor::create(ctx, start.getLoc(), init, cond, incr, blk, false);

	// StmtBlock
	Vector<Stmt *> outerblkstmts = {in_interm_vardecl, loop};
	fin			     = StmtBlock::create(ctx, start.getLoc(), outerblkstmts, false);
	return true;
}
bool Parsing::parseFor(ParseHelper &p, Stmt *&f)
{
	f = nullptr;

	Stmt *init	   = nullptr; // either of StmtVarDecl or StmtExpr
	Stmt *cond	   = nullptr;
	Stmt *incr	   = nullptr;
	StmtBlock *blk	   = nullptr;
	bool is_inline	   = false;
	lex::Lexeme &start = p.peek();

	if(p.acceptn(lex::INLINE)) is_inline = true;

	if(!p.acceptn(lex::FOR)) {
		err::out(p.peek(), "expected 'for' here, found: ", p.peek().getTok().cStr());
		return false;
	}

init:
	if(p.acceptn(lex::COLS)) goto cond;

	if(p.accept(lex::LET)) {
		if(!parseVarDecl(p, init)) return false;
	} else {
		if(!parseExpr(p, init, false)) return false;
	}
	if(!p.acceptn(lex::COLS)) {
		err::out(p.peek(), "expected semicolon here, found: ", p.peek().getTok().cStr());
		return false;
	}

cond:
	if(p.acceptn(lex::COLS)) goto incr;

	if(!parseExpr16(p, cond, false)) return false;
	if(!p.acceptn(lex::COLS)) {
		err::out(p.peek(), "expected semicolon here, found: ", p.peek().getTok().cStr());
		return false;
	}

incr:
	if(p.accept(lex::LBRACE)) goto body;

	if(!parseExpr(p, incr, true)) return false;
	if(!p.accept(lex::LBRACE)) {
		err::out(p.peek(),
			 "expected braces for body here, found: ", p.peek().getTok().cStr());
		return false;
	}

body:
	if(!parseBlock(p, blk)) {
		err::out(p.peek(), "failed to parse block for 'for' construct");
		return false;
	}

	f = StmtFor::create(ctx, start.getLoc(), init, cond, incr, blk, is_inline);
	return true;
}
bool Parsing::parseWhile(ParseHelper &p, Stmt *&w)
{
	w = nullptr;

	Stmt *cond	   = nullptr;
	StmtBlock *blk	   = nullptr;
	bool is_inline	   = false;
	lex::Lexeme &start = p.peek();

	if(p.acceptn(lex::INLINE)) is_inline = true;

	if(!p.acceptn(lex::WHILE)) {
		err::out(p.peek(), "expected 'while' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseExpr16(p, cond, true)) return false;

	if(!parseBlock(p, blk)) {
		err::out(p.peek(), "failed to parse block for 'for' construct");
		return false;
	}

	w = StmtFor::create(ctx, start.getLoc(), nullptr, cond, nullptr, blk, is_inline);
	return true;
}
bool Parsing::parseReturn(ParseHelper &p, Stmt *&ret)
{
	ret = nullptr;

	Stmt *val	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::RETURN)) {
		err::out(p.peek(), "expected 'return' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(p.accept(lex::COLS)) goto done;

	if(!parseExpr16(p, val, false)) {
		err::out(p.peek(), "failed to parse expression for return value");
		return false;
	}

done:
	ret = StmtRet::create(ctx, start.getLoc(), val);
	return true;
}
bool Parsing::parseContinue(ParseHelper &p, Stmt *&cont)
{
	cont = nullptr;

	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::CONTINUE)) {
		err::out(p.peek(), "expected 'continue' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	cont = StmtContinue::create(ctx, start.getLoc());
	return true;
}
bool Parsing::parseBreak(ParseHelper &p, Stmt *&brk)
{
	brk = nullptr;

	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::BREAK)) {
		err::out(p.peek(), "expected 'break' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	brk = StmtBreak::create(ctx, start.getLoc());
	return true;
}
bool Parsing::parseDefer(ParseHelper &p, Stmt *&defer)
{
	defer = nullptr;

	Stmt *val	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::DEFER)) {
		err::out(p.peek(), "expected 'defer' here, found: ", p.peek().getTok().cStr());
		return false;
	}

	if(!parseExpr16(p, val, false)) {
		err::out(p.peek(), "failed to parse expression for return value");
		return false;
	}

done:
	defer = StmtDefer::create(ctx, start.getLoc(), val);
	return true;
}
} // namespace sc
