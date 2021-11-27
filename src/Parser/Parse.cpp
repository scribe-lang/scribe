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

#include "Parser/Parse.hpp"

#include <unordered_set>

#include "Error.hpp"

namespace sc
{
Parsing::Parsing(ErrMgr &err, Context &ctx) : err(err), ctx(ctx) {}

// on successful parse, returns true, and tree is allocated
// if with_brace is true, it will attempt to find the beginning and ending brace for each block
bool Parsing::parse_block(ParseHelper &p, StmtBlock *&tree, const bool &with_brace)
{
	tree = nullptr;

	std::vector<Stmt *> stmts;
	Stmt *stmt = nullptr;

	lex::Lexeme &start = p.peak();

	if(with_brace) {
		if(!p.acceptn(lex::LBRACE)) {
			err.set(p.peak(), "expected opening braces '{' for block, found: %s",
				p.peak().getTok().cStr());
			return false;
		}
	}

	while(p.isValid() && (!with_brace || !p.accept(lex::RBRACE))) {
		// TODO: this must be during simplify as all inline stuff is resolved as well
		// if(!with_brace && !p.accept(lex::LET)) {
		// 	err.set(p.peak(), "top level block can contain only 'let' declarations");
		// 	return false;
		// }
		bool skip_cols = false;
		// logic
		if(p.accept(lex::LET)) {
			if(!parse_vardecl(p, stmt)) return false;
		} else if(p.accept(lex::IF)) {
			if(!parse_conds(p, stmt)) return false;
			skip_cols = true;
		} else if(p.accept(lex::INLINE)) {
			if(p.peakt(1) == lex::FOR) {
				if(!parse_for(p, stmt)) return false;
				skip_cols = true;
			} else if(p.peakt(1) == lex::IF) {
				if(!parse_conds(p, stmt)) return false;
				skip_cols = true;
			} else {
				err.set(p.peak(1), "'inline' is not applicable on '%s' statement",
					p.peak(1).getTok().cStr());
				return false;
			}
		} else if(p.accept(lex::FOR)) {
			if(p.peakt(1) == lex::IDEN && p.peakt(2) == lex::IN) {
				if(!parse_forin(p, stmt)) return false;
			} else {
				if(!parse_for(p, stmt)) return false;
			}
			skip_cols = true;
		} else if(p.accept(lex::WHILE)) {
			if(!parse_while(p, stmt)) return false;
			skip_cols = true;
		} else if(p.accept(lex::RETURN)) {
			if(!parse_ret(p, stmt)) return false;
		} else if(p.accept(lex::CONTINUE)) {
			if(!parse_continue(p, stmt)) return false;
		} else if(p.accept(lex::BREAK)) {
			if(!parse_break(p, stmt)) return false;
		} else if(p.accept(lex::DEFER)) {
			if(!parse_defer(p, stmt)) return false;
		} else if(p.accept(lex::LBRACE)) {
			if(!parse_block(p, (StmtBlock *&)stmt)) return false;
			skip_cols = true;
		} else if(!parse_expr(p, stmt, false)) {
			return false;
		}

		if(skip_cols || p.acceptn(lex::COLS)) {
			stmts.push_back(stmt);
			stmt = nullptr;
			continue;
		}
		err.set(p.peak(), "expected semicolon for end of statement, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

	if(with_brace) {
		if(!p.acceptn(lex::RBRACE)) {
			err.set(p.peak(), "expected closing braces '}' for block, found: %s",
				p.peak().getTok().cStr());
			return false;
		}
	}

	tree = StmtBlock::create(ctx, start.getLoc(), stmts, !with_brace);
	return true;
}

bool Parsing::parse_type(ParseHelper &p, StmtType *&type)
{
	type = nullptr;

	size_t ptr    = 0;
	size_t info   = 0;
	Stmt *count   = nullptr;
	Stmt *expr    = nullptr;
	bool dot_turn = false; // to ensure type name is in the form <name><dot><name>...

	lex::Lexeme &start = p.peak();

	if(p.accept(lex::COMPTIME, lex::FN)) {
		if(!parse_fnsig(p, expr)) return false;
		type = StmtType::create(ctx, start.getLoc(), 0, 0, expr);
		return true;
	}

	if(p.acceptn(lex::PreVA)) info |= TypeInfoMask::VARIADIC;

	while(p.acceptn(lex::MUL)) ++ptr;

	if(p.acceptn(lex::BAND)) info |= TypeInfoMask::REF;
	if(p.acceptn(lex::STATIC)) info |= TypeInfoMask::STATIC;
	if(p.acceptn(lex::CONST)) info |= TypeInfoMask::CONST;
	if(p.acceptn(lex::VOLATILE)) info |= TypeInfoMask::VOLATILE;

	if(!parse_expr_01(p, expr, true)) {
		err.set(p.peak(), "failed to parse type expression");
		return false;
	}

	type = StmtType::create(ctx, start.getLoc(), ptr, info, expr);
	return true;
fail:
	if(count) delete count;
	if(expr) delete expr;
	return false;
}

bool Parsing::parse_simple(ParseHelper &p, Stmt *&data)
{
	data = nullptr;

	if(!p.peak().getTok().isData()) {
		err.set(p.peak(), "expected data here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	lex::Lexeme &val = p.peak();
	p.next();

	data = StmtSimple::create(ctx, val.getLoc(), val);
	return true;
}

bool Parsing::parse_expr(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	return parse_expr_17(p, expr, disable_brace_after_iden);
}

// Left Associative
// ,
bool Parsing::parse_expr_17(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	size_t commas = 0;

	if(!parse_expr_16(p, rhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::COMMA)) {
		++commas;
		oper = p.peak();
		p.next();
		if(!parse_expr_16(p, lhs, disable_brace_after_iden)) {
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
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// ?:
bool Parsing::parse_expr_16(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs     = nullptr;
	Stmt *rhs     = nullptr;
	Stmt *lhs_lhs = nullptr;
	Stmt *lhs_rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_15(p, lhs, disable_brace_after_iden)) {
		return false;
	}
	if(!p.accept(lex::QUEST)) {
		expr = lhs;
		return true;
	}

	oper = p.peak();
	p.next();
	lex::Lexeme oper_inside;

	if(!parse_expr_15(p, lhs_lhs, disable_brace_after_iden)) {
		return false;
	}
	if(!p.accept(lex::COL)) {
		err.set(p.peak(), "expected ':' for ternary operator, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	oper_inside = p.peak();
	p.next();
	if(!parse_expr_15(p, lhs_rhs, disable_brace_after_iden)) {
		return false;
	}
	rhs = StmtExpr::create(ctx, oper.getLoc(), 0, lhs_lhs, oper_inside, lhs_rhs, false);
	goto after_quest;

after_quest:
	expr = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
	return true;
fail:
	if(lhs_rhs) delete lhs_rhs;
	if(lhs_lhs) delete lhs_lhs;
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Right Associative
// =
bool Parsing::parse_expr_15(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_14(p, rhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ASSN)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_14(p, lhs, disable_brace_after_iden)) {
			return false;
		}
		rhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		lhs = nullptr;
	}

	expr = rhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// += -=
// *= /= %=
// <<= >>=
// &= |= ^=
// or-block
bool Parsing::parse_expr_14(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs	  = nullptr;
	Stmt *rhs	  = nullptr;
	StmtBlock *or_blk = nullptr;
	lex::Lexeme or_blk_var;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_13(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ADD_ASSN, lex::SUB_ASSN, lex::MUL_ASSN) ||
	      p.accept(lex::DIV_ASSN, lex::MOD_ASSN, lex::LSHIFT_ASSN) ||
	      p.accept(lex::RSHIFT_ASSN, lex::BAND_ASSN, lex::BOR_ASSN) ||
	      p.accept(lex::BNOT_ASSN, lex::BXOR_ASSN))
	{
		oper = p.peak();
		p.next();
		if(!parse_expr_13(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;

	if(!p.acceptn(lex::OR)) return true;

	if(p.accept(lex::IDEN)) {
		or_blk_var = p.peak();
		p.next();
	}

	if(!parse_block(p, or_blk)) {
		return false;
	}
	if(expr->getStmtType() != EXPR) {
		expr = StmtExpr::create(ctx, expr->getLoc(), 0, expr, {}, nullptr, false);
	}
	as<StmtExpr>(expr)->setOr(or_blk, or_blk_var);
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	if(or_blk) delete or_blk;
	return false;
}
// Left Associative
// ||
bool Parsing::parse_expr_13(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_12(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LOR)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_12(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// &&
bool Parsing::parse_expr_12(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_11(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LAND)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_11(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// |
bool Parsing::parse_expr_11(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_10(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BOR)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_10(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// ^
bool Parsing::parse_expr_10(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_09(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BAND)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_09(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// &
bool Parsing::parse_expr_09(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_08(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::BAND)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_08(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// == !=
bool Parsing::parse_expr_08(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_07(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::EQ, lex::NE)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_07(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// < <=
// > >=
bool Parsing::parse_expr_07(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_06(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LT, lex::LE) || p.accept(lex::GT, lex::GE)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_06(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// << >>
bool Parsing::parse_expr_06(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_05(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::LSHIFT, lex::RSHIFT)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_05(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// + -
bool Parsing::parse_expr_05(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_04(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::ADD, lex::SUB)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_04(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Left Associative
// * / %
bool Parsing::parse_expr_04(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;

	lex::Lexeme oper;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_03(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	while(p.accept(lex::MUL, lex::DIV, lex::MOD)) {
		oper = p.peak();
		p.next();
		if(!parse_expr_03(p, rhs, disable_brace_after_iden)) {
			return false;
		}
		lhs = StmtExpr::create(ctx, start.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	return false;
}
// Right Associative (single operand)
// ++ -- (pre)
// + - (unary)
// * & (deref, addrof)
// ! ~ (log/bit)
bool Parsing::parse_expr_03(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;

	std::vector<lex::Lexeme> opers;

	lex::Lexeme &start = p.peak();

	while(p.accept(lex::XINC, lex::XDEC) || p.accept(lex::ADD, lex::SUB) ||
	      p.accept(lex::MUL, lex::BAND) || p.accept(lex::LNOT, lex::BNOT))
	{
		if(p.peakt() == lex::XINC) p.sett(lex::INCX);
		if(p.peakt() == lex::XDEC) p.sett(lex::DECX);
		if(p.peakt() == lex::ADD) p.sett(lex::UADD);
		if(p.peakt() == lex::SUB) p.sett(lex::USUB);
		if(p.peakt() == lex::MUL) p.sett(lex::UMUL);
		if(p.peakt() == lex::BAND) p.sett(lex::UAND);
		opers.insert(opers.begin(), p.peak());
		p.next();
	}

	if(!parse_expr_02(p, lhs, disable_brace_after_iden)) {
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
fail:
	if(lhs) delete lhs;
	return false;
}
// Left Associative
// ++ -- (post)
// ... (postva)
bool Parsing::parse_expr_02(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	Stmt *lhs = nullptr;

	std::vector<lex::Lexeme> opers;

	lex::Lexeme &start = p.peak();

	if(!parse_expr_01(p, lhs, disable_brace_after_iden)) {
		return false;
	}

	if(p.accept(lex::XINC, lex::XDEC, lex::PreVA)) {
		if(p.peakt() == lex::PreVA) p.sett(lex::PostVA);
		lhs = StmtExpr::create(ctx, p.peak().getLoc(), 0, lhs, p.peak(), nullptr, false);
		p.next();
	}

	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	return false;
}
bool Parsing::parse_expr_01(ParseHelper &p, Stmt *&expr, const bool &disable_brace_after_iden)
{
	expr = nullptr;

	lex::Lexeme &start = p.peak();
	lex::Lexeme dot;

	Stmt *lhs = nullptr;
	Stmt *rhs = nullptr;
	std::vector<Stmt *> args;
	Stmt *arg	  = nullptr;
	bool is_intrinsic = false;

	if(p.acceptn(lex::LPAREN)) {
		if(!parse_expr(p, lhs, disable_brace_after_iden)) {
			return false;
		}
		if(!p.acceptn(lex::RPAREN)) {
			err.set(p.peak(),
				"expected ending parenthesis ')' for expression, found: %s",
				p.peak().getTok().cStr());
			return false;
		}
	}

begin:
	if(p.acceptn(lex::AT)) is_intrinsic = true;
	if(p.acceptd() && !parse_simple(p, lhs)) return false;
	goto begin_brack;

after_dot:
	if(!p.acceptd() || !parse_simple(p, rhs)) return false;
	if(lhs && rhs) {
		lhs = StmtExpr::create(ctx, dot.getLoc(), 0, lhs, dot, rhs, false);
		rhs = nullptr;
	}

begin_brack:
	if(p.accept(lex::LBRACK)) {
		lex::Lexeme oper;
		p.sett(lex::SUBS);
		oper = p.peak();
		p.next();
		if(is_intrinsic) {
			err.set(p.peak(), "only function calls can be intrinsic;"
					  " attempted subscript here");
			return false;
		}
		if(!parse_expr_16(p, rhs, false)) {
			err.set(oper, "failed to parse expression for subscript");
			return false;
		}
		if(!p.acceptn(lex::RBRACK)) {
			err.set(p.peak(),
				"expected closing bracket for"
				" subscript expression, found: %s",
				p.peak().getTok().cStr());
			return false;
		}
		lhs = StmtExpr::create(ctx, oper.getLoc(), 0, lhs, oper, rhs, false);
		rhs = nullptr;
		if(p.accept(lex::LBRACK, lex::LPAREN) ||
		   (p.peakt() == lex::DOT && p.peakt(1) == lex::LT))
			goto begin_brack;
	} else if(p.accept(lex::LPAREN) || (!disable_brace_after_iden && p.accept(lex::LBRACE))) {
		bool fncall = p.accept(lex::LPAREN);
		lex::Lexeme oper;
		if(!p.accept(lex::LPAREN, lex::LBRACE)) {
			err.set(p.peak(),
				"expected opening parenthesis/brace"
				" for function/struct call, found: %s",
				p.peak().getTok().cStr());
			return false;
		}
		p.sett(fncall ? lex::FNCALL : lex::STCALL);
		oper = p.peak();
		p.next();
		if(p.acceptn(fncall ? lex::RPAREN : lex::RBRACE)) {
			goto post_args;
		}
		// parse arguments
		while(true) {
			if(!parse_expr_16(p, arg, false)) return false;
			args.push_back(arg);
			arg = nullptr;
			if(!p.acceptn(lex::COMMA)) break;
		}
		if(!p.acceptn(fncall ? lex::RPAREN : lex::RBRACE)) {
			err.set(p.peak(),
				"expected closing parenthesis/brace after function/struct"
				" call arguments, found: %s",
				p.peak().getTok().cStr());
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
			StmtExpr::create(ctx, p.peak(-1).getLoc(), 0, lhs, p.peak(-1), rhs, false);
			rhs = nullptr;
		}
		dot = p.peak(-1);
		goto after_dot;
	}

done:
	if(lhs && rhs) {
		lhs = StmtExpr::create(ctx, dot.getLoc(), 0, lhs, dot, rhs, false);
		rhs = nullptr;
	}
	expr = lhs;
	return true;
fail:
	if(lhs) delete lhs;
	if(rhs) delete rhs;
	for(auto &a : args) delete a;
	if(arg) delete arg;
	return false;
}

bool Parsing::parse_var(ParseHelper &p, StmtVar *&var, const Occurs &intype, const Occurs &otype,
			const Occurs &oval)
{
	var = nullptr;

	bool comptime = false;
	bool global   = false;
	while(p.accept(lex::COMPTIME, lex::GLOBAL)) {
		if(p.acceptn(lex::COMPTIME)) comptime = true;
		if(p.acceptn(lex::GLOBAL)) global = true;
	}

	if(!p.accept(lex::IDEN)) {
		err.set(p.peak(), "expected identifier for variable name, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	lex::Lexeme &name = p.peak();
	p.next();
	StmtType *in   = nullptr;
	StmtType *type = nullptr;
	Stmt *val      = nullptr;

in:
	if(intype == Occurs::NO && p.accept(lex::IN)) {
		err.set(p.peak(), "unexpected 'in' here");
		return false;
	}
	if(!p.acceptn(lex::IN)) {
		goto type;
	}
	if(comptime) {
		err.set(p.peak(), "comptime can be used only for data variables");
		return false;
	}
	if(!parse_type(p, in)) {
		err.set(p.peak(), "failed to parse in-type for variable: %s",
			name.getDataStr().c_str());
		return false;
	}

type:
	if(otype == Occurs::NO && p.accept(lex::COL)) {
		err.set(p.peak(), "unexpected beginning of type here");
		return false;
	}
	if(!p.acceptn(lex::COL)) {
		goto val;
	}
	if(!parse_type(p, type)) {
		err.set(p.peak(), "failed to parse type for variable: %s",
			name.getDataStr().c_str());
		return false;
	}
	if(type->isMetaType() && !comptime) {
		err.set(type, "a variable of type 'type' must be comptime");
		return false;
	}

val:
	if(oval == Occurs::NO && p.accept(lex::ASSN)) {
		err.set(p.peak(), "unexpected beginning of value assignment here");
		return false;
	}
	if(!p.acceptn(lex::ASSN)) {
		goto done;
	}
	if(comptime && (p.accept(lex::ENUM, lex::STRUCT) || p.accept(lex::FN, lex::EXTERN))) {
		err.set(p.peak(), "comptime declaration can only have an expression as value");
		return false;
	}
	if(p.accept(lex::ENUM)) {
		if(!parse_enum(p, val)) return false;
	} else if(p.accept(lex::STRUCT)) {
		if(!parse_struct(p, val, true)) return false;
	} else if(p.accept(lex::FN)) {
		if(!parse_fndef(p, val)) return false;
	} else if(p.accept(lex::EXTERN)) {
		if(!parse_extern(p, val)) return false;
		if(!as<StmtExtern>(val)->getEntity() && !type) {
			err.set(name, "variable extern must have a type");
			return false;
		}
	} else if(!parse_expr_16(p, val, false)) {
		return false;
	}

done:
	if(!type && !val) {
		err.set(name, "invalid variable declaration - no type or value set");
		return false;
	}
	if(comptime && !val && oval != Occurs::NO) {
		err.set(name, "comptime variable cannot be declared without an expression");
		return false;
	}
	if(in) {
		if(type) {
			err.set(name, "let-in statements can only have values (function "
				      "definitions) - no types allowed");
			return false;
		}
		if(val && val->getStmtType() != FNDEF) {
			err.set(name, "only functions can be created using let-in statements");
			return false;
		}
		lex::Lexeme selfeme(in->getLoc(), lex::IDEN, "self");
		in->addTypeInfoMask(REF);
		StmtVar *selfvar =
		StmtVar::create(ctx, in->getLoc(), selfeme, in, nullptr, false, false, false);
		StmtFnSig *valsig = as<StmtFnDef>(val)->getSig();
		valsig->getArgs().insert(valsig->getArgs().begin(), selfvar);
	}
	var = StmtVar::create(ctx, name.getLoc(), name, type, val, in, comptime, global);
	return true;
fail:
	if(in) delete in;
	if(type) delete type;
	if(val) delete val;
	return false;
}

bool Parsing::parse_fnsig(ParseHelper &p, Stmt *&fsig)
{
	fsig = nullptr;

	std::vector<StmtVar *> args;
	StmtVar *var = nullptr;
	std::unordered_set<std::string> argnames;
	bool found_va	   = false;
	StmtType *rettype  = nullptr;
	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::FN)) {
		err.set(p.peak(), "expected 'fn' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	if(!p.acceptn(lex::LPAREN)) {
		err.set(p.peak(), "expected opening parenthesis for function args, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	if(p.acceptn(lex::RPAREN)) goto post_args;

	// args
	while(true) {
		bool is_comptime = false;
		if(p.acceptn(lex::COMPTIME)) is_comptime = true;
		if(argnames.find(p.peak().getDataStr()) != argnames.end()) {
			err.set(p.peak(), "this argument name is already used "
					  "before in this function signature");
			return false;
		}
		argnames.insert(p.peak().getDataStr());
		if(is_comptime) p.setPos(p.getPos() - 1);
		if(!parse_var(p, var, Occurs::NO, Occurs::YES, Occurs::NO)) {
			return false;
		}
		if(var->getVType()->hasModifier(VARIADIC)) {
			found_va = true;
		}
		Stmt *vtexpr = var->getVType()->getExpr();
		if(vtexpr->getStmtType() == SIMPLE &&
		   as<StmtSimple>(vtexpr)->getLexValue().getTokVal() == lex::ANY && !found_va)
		{
			err.set(vtexpr, "type 'any' can be only used for variadic functions");
			return false;
		}
		args.push_back(var);
		var = nullptr;
		if(!p.acceptn(lex::COMMA)) break;
		if(found_va) {
			err.set(p.peak(), "no parameter can exist after variadic");
			return false;
		}
	}

	if(!p.acceptn(lex::RPAREN)) {
		err.set(p.peak(), "expected closing parenthesis after function args, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

post_args:
	if(p.acceptn(lex::COL) && !parse_type(p, rettype)) {
		err.set(p.peak(), "failed to parse return type for function");
		return false;
	}
	if(!rettype) {
		lex::Lexeme voideme = lex::Lexeme(p.peak(-1).getLoc(), lex::VOID, "void");
		StmtSimple *voidsim = StmtSimple::create(ctx, voideme.getLoc(), voideme);
		rettype		    = StmtType::create(ctx, voidsim->getLoc(), 0, 0, voidsim);
	}

	fsig = StmtFnSig::create(ctx, start.getLoc(), args, rettype, 0, found_va);
	return true;
fail:
	if(var) delete var;
	for(auto &p : args) delete p;
	if(rettype) delete rettype;
	return false;
}
bool Parsing::parse_fndef(ParseHelper &p, Stmt *&fndef)
{
	fndef = nullptr;

	Stmt *sig	   = nullptr;
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peak();

	if(!parse_fnsig(p, sig)) return false;
	if(!parse_block(p, blk)) return false;

	fndef = StmtFnDef::create(ctx, start.getLoc(), (StmtFnSig *)sig, blk);
	return true;
fail:
	if(sig) delete sig;
	if(blk) delete blk;
	return false;
}

bool Parsing::parse_header(ParseHelper &p, StmtHeader *&header)
{
	header = nullptr;

	lex::Lexeme names, flags;

	if(!p.accept(lex::IDEN, lex::STR)) {
		err.set(p.peak(), "expected string or identifier for the name of header, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	names = p.peak();
	p.next();

	if(!p.acceptn(lex::COL)) goto done;

	if(!p.accept(lex::IDEN, lex::STR)) {
		err.set(p.peak(), "expected string or identifier for the header flags, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	flags = p.peak();
	p.next();

done:
	header = StmtHeader::create(ctx, names.getLoc(), names, flags);
	return true;
}
bool Parsing::parse_lib(ParseHelper &p, StmtLib *&lib)
{
	lib = nullptr;

	lex::Lexeme flags;

	if(!p.accept(lex::IDEN, lex::STR)) {
		err.set(p.peak(), "expected string or identifier for the lib flags, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	flags = p.peak();
	p.next();

	lib = StmtLib::create(ctx, flags.getLoc(), flags);
	return true;
}
bool Parsing::parse_extern(ParseHelper &p, Stmt *&ext)
{
	ext = nullptr;

	lex::Lexeme name; // name of the function
	StmtHeader *headers = nullptr;
	StmtLib *libs	    = nullptr;
	Stmt *entity	    = nullptr;
	bool struct_kw	    = false;

	if(!p.acceptn(lex::EXTERN)) {
		err.set(p.peak(), "expected extern keyword here, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

	if(!p.acceptn(lex::LBRACK)) {
		err.set(p.peak(), "expected opening bracket for extern information, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

	if(p.acceptn(lex::STRUCT)) struct_kw = true;
	if(!p.accept(lex::IDEN)) {
		err.set(p.peak(), "expected identifier or string here, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	name = p.peak();
	p.next();

	// This is for handling typedefs and structs in C
	// as 'struct' needs to be prepended for structs
	if(struct_kw) name.setDataStr("struct " + name.getDataStr());

	if(!p.acceptn(lex::COMMA)) goto endinfo;

	if(!parse_header(p, headers)) return false;
	if(!p.acceptn(lex::COMMA)) goto endinfo;
	if(!parse_lib(p, libs)) return false;

endinfo:
	if(!p.acceptn(lex::RBRACK)) {
		err.set(p.peak(), "expected closing bracket after extern information, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

	if(p.accept(lex::FN)) {
		if(!parse_fnsig(p, entity)) return false;
	} else if(p.accept(lex::STRUCT)) {
		if(!parse_struct(p, entity, false)) return false;
	}

end:
	ext = StmtExtern::create(ctx, name.getLoc(), name, headers, libs, entity);
	return true;
fail:
	if(headers) delete headers;
	if(libs) delete libs;
	if(entity) delete entity;
	return false;
}

bool Parsing::parse_enum(ParseHelper &p, Stmt *&ed)
{
	std::vector<lex::Lexeme> enumvars;

	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::ENUM)) {
		err.set(p.peak(), "expected 'enum' keyword here, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	if(!p.acceptn(lex::LBRACE)) {
		err.set(p.peak(), "expected left braces for beginning of enum list, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	while(p.accept(lex::IDEN)) {
		enumvars.push_back(p.peak());
		p.next();
		if(!p.acceptn(lex::COMMA)) break;
	}
	if(!p.acceptn(lex::RBRACE)) {
		err.set(p.peak(), "expected right braces for end of enum list, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	if(enumvars.empty()) {
		err.set(start, "cannot have empty enumeration");
		return false;
	}
	ed = StmtEnum::create(ctx, start.getLoc(), enumvars);
	return true;
}

bool Parsing::parse_struct(ParseHelper &p, Stmt *&sd, const bool &allowed_templs)
{
	sd = nullptr;

	std::vector<StmtVar *> fields;
	std::vector<lex::Lexeme> templates;
	StmtVar *field = nullptr;
	std::unordered_set<std::string> fieldnames;
	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::STRUCT)) {
		err.set(p.peak(), "expected 'struct' keyword here, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

	if(p.acceptn(lex::LT)) {
		if(!allowed_templs) {
			err.set(p.peak(), "templates are not allowed in externed structs");
			return false;
		}
		while(p.accept(lex::IDEN)) {
			templates.push_back(p.peak());
			p.next();
			if(!p.acceptn(lex::COMMA)) break;
		}
		if(!p.acceptn(lex::GT)) {
			err.set(p.peak(), "expected '>' for end of struct template list, found: %s",
				p.peak().getTok().cStr());
			return false;
		}
	}

	if(!p.acceptn(lex::LBRACE)) {
		err.set(p.peak(), "expected opening braces for struct definition, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

	while(p.accept(lex::IDEN, lex::COMPTIME)) {
		if(!parse_var(p, field, Occurs::NO, Occurs::YES, Occurs::NO)) return false;
		if(fieldnames.find(field->getName().getDataStr()) != fieldnames.end()) {
			err.set(p.peak(), "this field name is already used "
					  "before in this same structure");
			return false;
		}
		fieldnames.insert(field->getName().getDataStr());
		fields.push_back(field);
		field = nullptr;
		if(!p.acceptn(lex::COLS)) break;
	}
	if(!p.acceptn(lex::RBRACE)) {
		err.set(p.peak(),
			"expected closing brace for struct/trait"
			" declaration/definition, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

done:
	sd = StmtStruct::create(ctx, start.getLoc(), fields, templates);
	return true;

fail:
	if(field) delete field;
	for(auto &f : fields) delete f;
	return false;
}

bool Parsing::parse_vardecl(ParseHelper &p, Stmt *&vd)
{
	vd = nullptr;

	std::vector<StmtVar *> decls;
	StmtVar *decl	   = nullptr;
	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::LET)) {
		err.set(p.peak(), "expected 'let' keyword here, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

	while(p.accept(lex::IDEN, lex::COMPTIME, lex::GLOBAL)) {
		bool global   = false;
		bool comptime = false;
		while(p.accept(lex::COMPTIME, lex::GLOBAL)) {
			if(p.acceptn(lex::COMPTIME)) comptime = true;
			if(p.acceptn(lex::GLOBAL)) global = true;
		}
		if(global) p.setPos(p.getPos() - 1);
		if(comptime) p.setPos(p.getPos() - 1);
		Occurs in  = comptime || global ? Occurs::NO : Occurs::MAYBE;
		Occurs val = comptime ? Occurs::YES : Occurs::MAYBE;
		if(!parse_var(p, decl, in, Occurs::MAYBE, val)) return false;
		decls.push_back(decl);
		decl = nullptr;
		if(!p.acceptn(lex::COMMA)) break;
	}

	vd = StmtVarDecl::create(ctx, start.getLoc(), decls);
	return true;

fail:
	if(decl) delete decl;
	for(auto &decl : decls) delete decl;
	return false;
}

bool Parsing::parse_conds(ParseHelper &p, Stmt *&conds)
{
	conds = nullptr;

	std::vector<Conditional> cvec;
	Conditional c(nullptr, nullptr);
	bool is_inline = false;

	if(p.acceptn(lex::INLINE)) is_inline = true;

	lex::Lexeme &start = p.peak();

cond:
	if(!p.acceptn(lex::IF)) {
		err.set(p.peak(), "expected 'if' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	if(!parse_expr_15(p, c.getCond(), true)) {
		err.set(p.peak(), "failed to parse condition for if/else if statement");
		return false;
	}

blk:
	if(!parse_block(p, c.getBlk())) {
		err.set(p.peak(), "failed to parse block for conditional");
		return false;
	}

	cvec.emplace_back(c.getCond(), c.getBlk());
	c.reset();

	if(p.peakt() == lex::ELSE) {
		p.next();
		if(p.peakt() == lex::IF) goto cond;
		goto blk;
	}

	conds = StmtCond::create(ctx, start.getLoc(), cvec, is_inline);
	return true;

fail:
	if(c.getCond()) delete c.getCond();
	if(c.getBlk()) delete c.getBlk();
	for(auto &ce : cvec) {
		if(ce.getCond()) delete ce.getCond();
		if(ce.getBlk()) delete ce.getBlk();
	}
	return false;
}
bool Parsing::parse_forin(ParseHelper &p, Stmt *&fin)
{
	fin = nullptr;

	lex::Lexeme iter;
	Stmt *in	   = nullptr; // L01
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::FOR)) {
		err.set(p.peak(), "expected 'for' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	if(!p.accept(lex::IDEN)) {
		err.set(p.peak(), "expected iterator (identifier) here, found: %s",
			p.peak().getTok().cStr());
		return false;
	}
	iter = p.peak();
	p.next();

	if(!p.acceptn(lex::IN)) {
		err.set(p.peak(), "expected 'in' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	if(!parse_expr_01(p, in, true)) {
		err.set(p.peak(), "failed to parse expression for 'in'");
		return false;
	}

	if(!p.accept(lex::LBRACE)) {
		err.set(p.peak(), "expected block for for-in construct, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

	if(!parse_block(p, blk)) {
		err.set(p.peak(), "failed to parse block for for-in construct");
		return false;
	}

	fin = StmtForIn::create(ctx, start.getLoc(), iter, in, blk);
	return true;
fail:
	if(in) delete in;
	if(blk) delete blk;
	return false;
}
bool Parsing::parse_for(ParseHelper &p, Stmt *&f)
{
	f = nullptr;

	Stmt *init	   = nullptr; // either of StmtVarDecl or StmtExpr
	Stmt *cond	   = nullptr;
	Stmt *incr	   = nullptr;
	StmtBlock *blk	   = nullptr;
	bool is_inline	   = false;
	lex::Lexeme &start = p.peak();

	if(p.acceptn(lex::INLINE)) is_inline = true;

	if(!p.acceptn(lex::FOR)) {
		err.set(p.peak(), "expected 'for' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

init:
	if(p.acceptn(lex::COLS)) goto cond;

	if(p.accept(lex::LET)) {
		if(!parse_vardecl(p, init)) return false;
	} else {
		if(!parse_expr(p, init, false)) return false;
	}
	if(!p.acceptn(lex::COLS)) {
		err.set(p.peak(), "expected semicolon here, found: %s", p.peak().getTok().cStr());
		return false;
	}

cond:
	if(p.acceptn(lex::COLS)) goto incr;

	if(!parse_expr_16(p, cond, false)) return false;
	if(!p.acceptn(lex::COLS)) {
		err.set(p.peak(), "expected semicolon here, found: %s", p.peak().getTok().cStr());
		return false;
	}

incr:
	if(p.accept(lex::LBRACE)) goto body;

	if(!parse_expr(p, incr, true)) return false;
	if(!p.accept(lex::LBRACE)) {
		err.set(p.peak(), "expected braces for body here, found: %s",
			p.peak().getTok().cStr());
		return false;
	}

body:
	if(!parse_block(p, blk)) {
		err.set(p.peak(), "failed to parse block for 'for' construct");
		return false;
	}

	f = StmtFor::create(ctx, start.getLoc(), init, cond, incr, blk, is_inline);
	return true;
fail:
	if(init) delete init;
	if(cond) delete cond;
	if(incr) delete incr;
	if(blk) delete blk;
	return false;
}
bool Parsing::parse_while(ParseHelper &p, Stmt *&w)
{
	w = nullptr;

	Stmt *cond	   = nullptr;
	StmtBlock *blk	   = nullptr;
	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::WHILE)) {
		err.set(p.peak(), "expected 'while' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	if(!parse_expr_16(p, cond, true)) return false;

	if(!parse_block(p, blk)) {
		err.set(p.peak(), "failed to parse block for 'for' construct");
		return false;
	}

	w = StmtWhile::create(ctx, start.getLoc(), cond, blk);
	return true;
fail:
	if(cond) delete cond;
	if(blk) delete blk;
	return false;
}
bool Parsing::parse_ret(ParseHelper &p, Stmt *&ret)
{
	ret = nullptr;

	Stmt *val	   = nullptr;
	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::RETURN)) {
		err.set(p.peak(), "expected 'return' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	if(p.accept(lex::COLS)) goto done;

	if(!parse_expr_16(p, val, false)) {
		err.set(p.peak(), "failed to parse expression for return value");
		return false;
	}

done:
	ret = StmtRet::create(ctx, start.getLoc(), val);
	return true;
fail:
	if(val) delete val;
	return false;
}
bool Parsing::parse_continue(ParseHelper &p, Stmt *&cont)
{
	cont = nullptr;

	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::CONTINUE)) {
		err.set(p.peak(), "expected 'continue' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	cont = StmtContinue::create(ctx, start.getLoc());
	return true;
}
bool Parsing::parse_break(ParseHelper &p, Stmt *&brk)
{
	brk = nullptr;

	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::BREAK)) {
		err.set(p.peak(), "expected 'break' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	brk = StmtBreak::create(ctx, start.getLoc());
	return true;
}
bool Parsing::parse_defer(ParseHelper &p, Stmt *&defer)
{
	defer = nullptr;

	Stmt *val	   = nullptr;
	lex::Lexeme &start = p.peak();

	if(!p.acceptn(lex::DEFER)) {
		err.set(p.peak(), "expected 'defer' here, found: %s", p.peak().getTok().cStr());
		return false;
	}

	if(!parse_expr_16(p, val, false)) {
		err.set(p.peak(), "failed to parse expression for return value");
		return false;
	}

done:
	defer = StmtDefer::create(ctx, start.getLoc(), val);
	return true;
fail:
	if(val) delete val;
	return false;
}
} // namespace sc
