// NOTE: Generates For-In and While loops as a for loop.

#include "AST/Parse.hpp"

namespace sc::AST
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

	// If braces are expected but not found, it will assume only 1 (expr) stmt will follow,
	// and only_one_stmt will be set to true.
	bool only_one_stmt = false;

	if(with_brace && !p.acceptn(lex::LBRACE)) only_one_stmt = true;

	StringMap<String> attributes;

	// if top stmt, push func to defer stack
	if(!with_brace) deferstack.pushFunc();
	deferstack.pushFrame(); // push frame for this block
	bool inserted_defers = false;

	while(p.isValid() && (!with_brace || !p.accept(lex::RBRACE) || only_one_stmt)) {
		bool skip_cols = false;

		if(p.accept(lex::ATTRS) && !parseAttributes(p, attributes)) return false;

		// TODO: this must be during simplify as all inline stuff is resolved as well
		// if(!with_brace && !p.accept(lex::LET)) {
		// 	err::out(p.peek(), "top level block can contain only 'let' declarations");
		// 	return false;
		// }

		if(p.accept(lex::LET)) {
			if(!parseVarDecl(p, stmt)) return false;
		} else if(p.accept(lex::IF)) {
			if(!parseConds(p, stmt)) return false;
			skip_cols = true;
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
					 p.peek(1).tokCStr(), "' statement");
				return false;
			}
		} else if(p.accept(lex::RETURN, lex::CONTINUE, lex::BREAK, lex::DEFER, lex::GOTO)) {
			if(!parseOneWord(p, stmt)) return false;
		} else if(p.accept(lex::LBRACE)) {
			if(!parseBlock(p, (StmtBlock *&)stmt)) return false;
			skip_cols = true;
		} else if(!parseExpr(p, stmt, false)) {
			return false;
		}

		if(!attributes.empty()) {
			stmt->setAttributes(std::move(attributes));
			attributes.clear();
		}

		if(!skip_cols && !only_one_stmt && !p.acceptn(lex::COLS)) {
			err::out(p.peek(), "expected semicolon for end of statement, found: ",
				 p.peek().tokCStr());
			return false;
		}
		// note: defer stmt does NOT set stmt variable
		if(stmt) {
			if((stmt->isOneWord() && as<StmtOneWord>(stmt)->isReturn()) &&
			   !inserted_defers)
			{
				Vector<Stmt *> deferred = deferstack.getAllStmts(ctx);
				stmts.insert(stmts.end(), deferred.begin(), deferred.end());
				inserted_defers = true;
			}
			// remove blocks at top level (also helps with conditional imports)
			if(!with_brace && stmt->isBlock()) {
				StmtBlock *blk		 = as<StmtBlock>(stmt);
				Vector<Stmt *> &blkstmts = blk->getStmts();
				stmts.insert(stmts.end(), blkstmts.begin(), blkstmts.end());
			} else {
				stmts.push_back(stmt);
			}
			stmt = nullptr;
		}
		if(only_one_stmt) break;
	}

	if(!inserted_defers) {
		Span<Stmt *> deferred = deferstack.getTopStmts(ctx);
		stmts.insert(stmts.end(), deferred.begin(), deferred.end());
	}

	deferstack.popFrame();
	if(!with_brace) deferstack.popFunc();

	if(with_brace && !only_one_stmt && !p.acceptn(lex::RBRACE)) {
		err::out(p.peek(),
			 "expected closing braces '}' for block, found: ", p.peek().tokCStr());
		return false;
	}

	tree = StmtBlock::create(ctx, start.getLoc(), stmts, !with_brace);
	return true;
}

bool Parsing::parseType(ParseHelper &p, StmtType *&type)
{
	StringMap<String> attrs;
	type = nullptr;

	uint16_t stmtmask = 0;
	Stmt *count	  = nullptr;
	Stmt *expr	  = nullptr;
	bool dotTurn	  = false; // to ensure type name is in the form <name><dot><name>...
	int pointerCount  = 0;

	lex::Lexeme &start = p.peek();

	if(p.accept(lex::FN, lex::STRUCT, lex::UNION, lex::EXTERN)) {
		if(!parseSignature(p, expr)) return false;
		type = StmtType::create(ctx, start.getLoc(), expr);
		return true;
	}

	if(p.acceptn(lex::PreVA)) attrs["variadic"] = "";

	while(p.acceptn(lex::MUL)) ++pointerCount;

	if(pointerCount > 0) attrs["pointerCount"] = toString(pointerCount);

	if(p.acceptn(lex::BAND)) attrs["reference"] = "";
	if(p.acceptn(lex::CONST)) attrs["constant"] = "";

	if(!parseExpr01(p, expr, true)) {
		err::out(p.peek(), "failed to parse type expression");
		return false;
	}

	if(!expr) {
		err::out(start, "no type expression found");
		return false;
	}

	type = StmtType::create(ctx, start.getLoc(), expr);
	if(!attrs.empty()) type->setAttributes(std::move(attrs));
	return true;
}

bool Parsing::parseSimple(ParseHelper &p, Stmt *&data)
{
	data = nullptr;

	if(!p.acceptd()) {
		err::out(p.peek(), "expected data here, found: ", p.peek().tokCStr());
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
	lex::Lexeme oper  = lex::Lexeme(iden.getLoc(), lex::FNCALL);

	p.next();
	p.next();

	StmtSimple *arg	   = StmtSimple::create(ctx, lit.getLoc(), lit);
	StmtSimple *fn	   = StmtSimple::create(ctx, iden.getLoc(), iden);
	StmtCallArgs *args = StmtCallArgs::create(ctx, arg->getLoc(), {arg});
	expr		   = StmtExpr::create(ctx, lit.getLoc(), 0, fn, oper, args, false);

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
			 "expected ':' for ternary operator, found: ", p.peek().tokCStr());
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
		if(val.isInt()) {
			while(!opers.empty() && opers.front().isType(lex::USUB)) {
				val.setDataInt(-val.getDataInt());
				opers.erase(opers.begin());
			}
		}
		if(val.isFlt()) {
			while(!opers.empty() && opers.front().isType(lex::USUB)) {
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
	if(p.accept(lex::IDEN) && p.peek(1).isLiteral() ||
	   p.peek().isLiteral() && p.peekt(1) == lex::IDEN)
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
				 p.peek().tokCStr());
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
				 p.peek().tokCStr());
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
				 p.peek().tokCStr());
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
				 p.peek().tokCStr());
			return false;
		}
	post_args:
		rhs  = StmtCallArgs::create(ctx, oper.getLoc(), args);
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
	StringMap<String> attrs;
	var = nullptr;

	if(p.accept(lex::ATTRS)) {
		if(!parseAttributes(p, attrs)) return false;
	}

	while(p.accept(lex::STATIC, lex::VOLATILE, lex::GLOBAL, lex::COMPTIME)) {
		if(p.acceptn(lex::COMPTIME)) attrs["comptime"] = "";
		if(p.acceptn(lex::STATIC)) attrs["static"] = "";
		if(p.acceptn(lex::VOLATILE)) attrs["volatile"] = "";
		if(p.acceptn(lex::GLOBAL)) attrs["global"] = "";
	}

	bool comptime = attrs.contains("comptime");
	bool stati    = attrs.contains("static");
	bool volatil  = attrs.contains("volatile");
	bool global   = attrs.contains("global");

	if(!p.accept(lex::IDEN)) {
		err::out(p.peek(),
			 "expected identifier for variable name, found: ", p.peek().tokCStr());
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
	attrs["inType"] = "";

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
	if(comptime && p.accept(lex::FN, lex::STRUCT, lex::UNION, lex::EXTERN)) {
		err::out(p.peek(), "comptime declaration can only have an expression as value");
		return false;
	}
	if(p.accept(lex::EXTERN) || p.accept(lex::STRUCT, lex::UNION)) {
		if(!parseSignature(p, val)) return false;
	} else if(p.accept(lex::INLINE, lex::FN)) {
		if(!parseFnDef(p, val)) return false;
	} else if(!parseExpr16(p, val, false)) {
		return false;
	}

done:
	if(!type && !val) {
		err::out(name, "invalid variable declaration - no type or value set");
		return false;
	}
	// Comptime variables must have a value, unless a type is required, in which case,
	// they are signature arguments which don't need to have values.
	if(comptime && !val && (oval != Occurs::NO && otype != Occurs::YES)) {
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
		in->addAttribute("reference");
		StmtVar *selfvar      = StmtVar::create(ctx, in->getLoc(), selfeme, in, nullptr);
		StmtSignature *valsig = as<StmtFnDef>(val)->getSig();
		valsig->getArgs().insert(valsig->getArgs().begin(), selfvar);
	}
	var = StmtVar::create(ctx, name.getLoc(), name, type, val);
	var->setAttributes(std::move(attrs));
	return true;
}

bool Parsing::parseSignature(ParseHelper &p, Stmt *&sig)
{
	sig = nullptr;

	StringMap<String> attrs;
	Vector<StmtVar *> args;
	StmtVar *var = nullptr;
	Set<StringRef> argnames;
	StmtType *rettype = nullptr;
	bool found_va	  = false;
	bool isExtern	  = p.acceptn(lex::EXTERN);

	lex::Lexeme &start  = p.peek();
	SignatureType sigty = SignatureType::FUNC;

	if(p.accept(lex::STRUCT)) sigty = SignatureType::STRUCT;
	else if(p.accept(lex::UNION)) sigty = SignatureType::UNION;

	if(!p.acceptn(lex::FN, lex::STRUCT, lex::UNION)) {
		err::out(p.peek(),
			 "expected 'fn', 'struct', or 'union' here, found: ", p.peek().tokCStr());
		return false;
	}

	if(!p.acceptn(lex::LPAREN)) {
		err::out(p.peek(), "expected opening parenthesis for function args, found: ",
			 p.peek().tokCStr());
		return false;
	}
	if(p.acceptn(lex::RPAREN)) goto post_args;

	// args
	while(true) {
		if(!parseVar(p, var, Occurs::NO, Occurs::YES, Occurs::MAYBE)) {
			return false;
		}
		if(argnames.find(var->getName().getDataStr()) != argnames.end()) {
			err::out(p.peek(), "argument name '", var->getName().getDataStr(),
				 "' is already used "
				 "before in this signature");
			return false;
		}
		argnames.insert(var->getName().getDataStr());
		if(var->getVType()->hasAttribute("variadic")) found_va = true;
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
			 p.peek().tokCStr());
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
		rettype		    = StmtType::create(ctx, voidsim->getLoc(), voidsim);
	}

	if(found_va) attrs["variadic"] = "";
	if(isExtern) attrs["extern"] = "";

	sig = StmtSignature::create(ctx, start.getLoc(), args, rettype, sigty);
	sig->setAttributes(std::move(attrs));
	return true;
}
bool Parsing::parseFnDef(ParseHelper &p, Stmt *&fndef)
{
	fndef = nullptr;

	StringMap<String> attrs;
	Stmt *sig	   = nullptr;
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(p.acceptn(lex::INLINE)) attrs["inline"] = "";

	deferstack.pushFunc();
	if(!parseSignature(p, sig)) return false;
	if(!p.accept(lex::LBRACE)) {
		err::out(p.peek(), "Expected block for func def, found: ", p.peek().tokCStr());
		return false;
	}
	if(!parseBlock(p, blk)) return false;
	deferstack.popFunc();

	fndef = StmtFnDef::create(ctx, start.getLoc(), (StmtSignature *)sig, blk);
	fndef->setAttributes(std::move(attrs));
	return true;
}

bool Parsing::parseVarDecl(ParseHelper &p, Stmt *&vd)
{
	vd = nullptr;

	Vector<StmtVar *> decls;
	StmtVar *decl	   = nullptr;
	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::LET)) {
		err::out(p.peek(), "expected 'let' keyword here, found: ", p.peek().tokCStr());
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
		err::out(p.peek(), "expected 'if' here, found: ", p.peek().tokCStr());
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

	conds = StmtCond::create(ctx, start.getLoc(), std::move(cvec), is_inline);
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
		err::out(p.peek(), "expected 'for' here, found: ", p.peek().tokCStr());
		return false;
	}

	if(!p.accept(lex::IDEN)) {
		err::out(p.peek(),
			 "expected iterator (identifier) here, found: ", p.peek().tokCStr());
		return false;
	}
	iter = p.peek();
	p.next();

	if(!p.acceptn(lex::IN)) {
		err::out(p.peek(), "expected 'in' here, found: ", p.peek().tokCStr());
		return false;
	}

	if(!parseExpr01(p, in, true)) {
		err::out(p.peek(), "failed to parse expression for 'in'");
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

	in_interm.setDataStr(toString(in_interm.getDataStr(), "_interm"));
	iter_interm.setDataStr(toString("_", iter_interm.getDataStr()));

	StmtVar *in_interm_var = StmtVar::create(ctx, in_interm.getLoc(), in_interm, nullptr, in);
	// block statement 1:
	StmtVarDecl *in_interm_vardecl = StmtVarDecl::create(ctx, loc, {in_interm_var});

	// init:
	// let <iter_interm> = <in_interm>.begin()
	StmtSimple *init_lhs	= StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *init_rhs	= StmtSimple::create(ctx, loc, lexbegin);
	StmtExpr *init_dot_expr = StmtExpr::create(ctx, loc, 0, init_lhs, dot_op, init_rhs, false);
	StmtCallArgs *init_call_info = StmtCallArgs::create(ctx, loc, {});
	StmtExpr *init_expr =
	StmtExpr::create(ctx, loc, 0, init_dot_expr, call_op, init_call_info, false);
	StmtVar *init_iter_interm_var =
	StmtVar::create(ctx, iter_interm.getLoc(), iter_interm, nullptr, init_expr);
	StmtVarDecl *init = StmtVarDecl::create(ctx, iter_interm.getLoc(), {init_iter_interm_var});

	// cond:
	// <iter_interm> != <in_interm>.end()
	StmtSimple *cond_rhs_lhs = StmtSimple::create(ctx, loc, in_interm);
	StmtSimple *cond_rhs_rhs = StmtSimple::create(ctx, loc, lexend);
	StmtExpr *cond_dot_expr =
	StmtExpr::create(ctx, loc, 0, cond_rhs_lhs, dot_op, cond_rhs_rhs, false);
	StmtCallArgs *cond_call_info = StmtCallArgs::create(ctx, loc, {});
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
	StmtSimple *incr_call_arg    = StmtSimple::create(ctx, loc, iter_interm);
	StmtCallArgs *incr_call_info = StmtCallArgs::create(ctx, loc, {incr_call_arg});
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
	StmtCallArgs *loop_var_val_call_info =
	StmtCallArgs::create(ctx, loc, {loop_var_val_call_arg});
	StmtExpr *loop_var_val	  = StmtExpr::create(ctx, loc, 0, loop_var_val_dot_expr, call_op,
						     loop_var_val_call_info, false);
	StmtVar *loop_var	  = StmtVar::create(ctx, loc, iter, nullptr, loop_var_val);
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
		err::out(p.peek(), "expected 'for' here, found: ", p.peek().tokCStr());
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
		err::out(p.peek(), "expected semicolon here, found: ", p.peek().tokCStr());
		return false;
	}

cond:
	if(p.acceptn(lex::COLS)) goto incr;

	if(!parseExpr16(p, cond, false)) return false;
	if(!p.acceptn(lex::COLS)) {
		err::out(p.peek(), "expected semicolon here, found: ", p.peek().tokCStr());
		return false;
	}

incr:
	if(p.accept(lex::LBRACE)) goto body;

	if(!parseExpr(p, incr, true)) return false;

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
		err::out(p.peek(), "expected 'while' here, found: ", p.peek().tokCStr());
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
bool Parsing::parseOneWord(ParseHelper &p, Stmt *&word)
{
	word	  = nullptr;
	Stmt *val = nullptr;

	lex::Lexeme &start = p.peek();

	if(!p.acceptn(lex::RETURN, lex::CONTINUE, lex::BREAK, lex::DEFER, lex::GOTO)) {
		err::out(p.peek(),
			 "expected 'return', 'continue', 'break', 'defer', or 'goto' here, found: ",
			 p.peek().tokCStr());
		return false;
	}

	OneWordType wordty;

	if(start.getType() == lex::RETURN) wordty = OneWordType::RETURN;
	else if(start.getType() == lex::CONTINUE) wordty = OneWordType::CONTINUE;
	else if(start.getType() == lex::BREAK) wordty = OneWordType::BREAK;
	else if(start.getType() == lex::DEFER) wordty = OneWordType::DEFER;
	else if(start.getType() == lex::GOTO) wordty = OneWordType::GOTO;

	if(p.accept(lex::COLS)) goto done;

	if(!parseExpr16(p, val, false)) {
		err::out(p.peek(), "failed to parse expression for return value");
		return false;
	}

done:
	word = StmtOneWord::create(ctx, start.getLoc(), val, wordty);
	return true;
}

bool Parsing::parseAttributes(ParseHelper &p, StringMap<String> &attrs)
{
	if(!p.accept(lex::ATTRS)) {
		err::out(p.peek(),
			 "expected attributes token to parse, found: ", p.peek().tokCStr());
		return false;
	}
	StringRef data	= p.peek().getDataStr();
	size_t keystart = -1, keylen = 0, valstart = -1, vallen = 0;
	bool isval = false;
	for(size_t i = 0; i < data.size(); ++i) {
		char c = data[i];
		if(isspace(c)) continue;
		if(c == ',') {
			if(keystart == -1) continue;
			StringRef key(&data[keystart], keylen);
			attrs[String(key)] =
			valstart == -1 ? "" : StringRef(&data[valstart], vallen);
			keystart = valstart = -1;
			keylen = vallen = 0;
			continue;
		}
		if(c == '=') {
			if(isval) {
				err::out(p.peek(), "encountered multiple assignments in attribute");
				return false;
			}
			isval = true;
			continue;
		}
		if(isval) {
			if(valstart == -1) valstart = i;
			++vallen;
		} else {
			if(keystart == -1) keystart = i;
			++keylen;
		}
	}
	if(keystart != -1) {
		StringRef key(&data[keystart], keylen);
		attrs[String(key)] = valstart == -1 ? "" : StringRef(&data[valstart], vallen);
	}
	p.next();
	return true;
}
} // namespace sc::AST
