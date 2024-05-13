#include "AST/Passes/ConstFold.hpp"

namespace sc::AST
{
ConstFoldPass::ConstFoldPass(Context &c) : Pass(Pass::genPassID<ConstFoldPass>(), c) {}
ConstFoldPass::~ConstFoldPass() {}

bool ConstFoldPass::visit(Stmt *stmt, Stmt **source)
{
	bool res = false;
	switch(stmt->getStmtType()) {
	case BLOCK: res = visit(as<StmtBlock>(stmt), source); break;
	case TYPE: res = visit(as<StmtType>(stmt), source); break;
	case SIMPLE: res = visit(as<StmtSimple>(stmt), source); break;
	case EXPR: res = visit(as<StmtExpr>(stmt), source); break;
	case FNCALLINFO: res = visit(as<StmtFnCallInfo>(stmt), source); break;
	case VAR: res = visit(as<StmtVar>(stmt), source); break;
	case FNSIG: res = visit(as<StmtFnSig>(stmt), source); break;
	case FNDEF: res = visit(as<StmtFnDef>(stmt), source); break;
	case HEADER: res = visit(as<StmtHeader>(stmt), source); break;
	case LIB: res = visit(as<StmtLib>(stmt), source); break;
	case EXTERN: res = visit(as<StmtExtern>(stmt), source); break;
	case ENUMDEF: res = visit(as<StmtEnum>(stmt), source); break;
	case STRUCTDEF: res = visit(as<StmtStruct>(stmt), source); break;
	case VARDECL: res = visit(as<StmtVarDecl>(stmt), source); break;
	case COND: res = visit(as<StmtCond>(stmt), source); break;
	case FOR: res = visit(as<StmtFor>(stmt), source); break;
	case RET: res = visit(as<StmtRet>(stmt), source); break;
	case CONTINUE: res = visit(as<StmtContinue>(stmt), source); break;
	case BREAK: res = visit(as<StmtBreak>(stmt), source); break;
	default: {
		err::out(stmt,
			 "invalid statement found for const folding: ", stmt->getStmtTypeCString());
		break;
	}
	}
	return res;
}
bool ConstFoldPass::visit(StmtBlock *stmt, Stmt **source)
{
	for(auto &s : stmt->getStmts()) {
		if(!visit(s, &s)) {
			err::out(s, "failed to generate IR for this statement");
			return false;
		}
	}
	return true;
}
bool ConstFoldPass::visit(StmtType *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtSimple *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtFnCallInfo *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtExpr *stmt, Stmt **source)
{
	Stmt *&lhs = stmt->getLHS();
	Stmt *&rhs = stmt->getRHS();
	if(lhs && !visit(lhs, &lhs)) return false;
	if(rhs && !visit(rhs, &rhs)) return false;

	if(!lhs || !rhs || !lhs->isSimple() || !rhs->isSimple()) return true;

	StmtSimple *l = as<StmtSimple>(lhs);
	StmtSimple *r = as<StmtSimple>(rhs);
	if(!l->getLexValue().isLiteral() || !r->getLexValue().isLiteral()) return true;

	const lex::Lexeme &oper = stmt->getOper();
	const ModuleLoc *loc	= oper.getLoc();

	switch(oper.getType()) {
	case lex::ARROW:
	case lex::DOT:
	case lex::FNCALL:
	case lex::STCALL:
	// address of
	case lex::UAND:
	// dereference
	case lex::UMUL:
	case lex::SUBS:
	case lex::ASSN: break;
	// Arithmetic
	case lex::ADD: {
		if(l->isLexTokType(lex::STR) && r->isLexTokType(lex::STR)) {
			String res(l->getLexValue().getDataStr());
			res += r->getLexValue().getDataStr();
			l->setLexeme(lex::Lexeme(loc, lex::STR, res));
			*source = l;
			break;
		}
		if(l->isLexTokType(lex::STR) || r->isLexTokType(lex::STR)) break;
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			lex::TokType resty = lex::F32;
			long double res =
			l->getLexValue().getDataAsFlt() + r->getLexValue().getDataAsFlt();
			if(res > lex::getFltMaxForType(resty)) resty = lex::F64;
			l->setLexeme(lex::Lexeme(loc, resty, res));
			*source = l;
			break;
		}
		lex::TokType resty = lex::I32;
		int64_t res = l->getLexValue().getDataAsInt() + r->getLexValue().getDataAsInt();
		if(res > lex::getIntMaxForType(resty)) resty = lex::I64;
		l->setLexeme(lex::Lexeme(loc, resty, res));
		*source = l;
		break;
	}
	case lex::SUB: {
		if(l->isLexTokType(lex::STR) || r->isLexTokType(lex::STR)) break;
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			lex::TokType resty = lex::F32;
			long double res =
			l->getLexValue().getDataAsFlt() - r->getLexValue().getDataAsFlt();
			if(res > lex::getFltMaxForType(resty) || res < lex::getFltMinForType(resty))
				resty = lex::F64;
			l->setLexeme(lex::Lexeme(loc, resty, res));
			*source = l;
			break;
		}
		lex::TokType resty = lex::I32;
		int64_t res = l->getLexValue().getDataAsInt() - r->getLexValue().getDataAsInt();
		if(res > lex::getIntMaxForType(resty) || res < lex::getIntMinForType(resty))
			resty = lex::I64;
		l->setLexeme(lex::Lexeme(loc, resty, res));
		*source = l;
		break;
	}
	case lex::MUL: {
		if(l->isLexTokType(lex::STR)) {
			if(!r->getLexValue().isInt()) break;
			String res;
			for(size_t i = 0; i < r->getLexValue().getDataInt(); ++i) {
				res += l->getLexValue().getDataStr();
			}
			l->setLexeme(lex::Lexeme(loc, lex::STR, res));
			*source = l;
			break;
		}
		if(r->isLexTokType(lex::STR)) {
			if(!l->getLexValue().isInt()) break;
			String res;
			for(size_t i = 0; i < l->getLexValue().getDataInt(); ++i) {
				res += r->getLexValue().getDataStr();
			}
			l->setLexeme(lex::Lexeme(loc, lex::STR, res));
			*source = l;
			break;
		}
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			lex::TokType resty = lex::F32;
			long double res =
			l->getLexValue().getDataAsFlt() * r->getLexValue().getDataAsFlt();
			if(res > lex::getFltMaxForType(resty) || res < lex::getFltMinForType(resty))
				resty = lex::F64;
			l->setLexeme(lex::Lexeme(loc, resty, res));
			*source = l;
			break;
		}
		lex::TokType resty = lex::I32;
		int64_t res = l->getLexValue().getDataAsInt() * r->getLexValue().getDataAsInt();
		if(res > lex::getIntMaxForType(resty) || res < lex::getIntMinForType(resty))
			resty = lex::I64;
		l->setLexeme(lex::Lexeme(loc, resty, res));
		*source = l;
		break;
	}
	case lex::DIV: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			lex::TokType resty = lex::F32;
			long double res =
			l->getLexValue().getDataAsFlt() / r->getLexValue().getDataAsFlt();
			if(res > lex::getFltMaxForType(resty) || res < lex::getFltMinForType(resty))
				resty = lex::F64;
			l->setLexeme(lex::Lexeme(loc, resty, res));
			*source = l;
			break;
		}
		lex::TokType resty = lex::I32;
		int64_t res = l->getLexValue().getDataAsInt() / r->getLexValue().getDataAsInt();
		if(res > lex::getIntMaxForType(resty) || res < lex::getIntMinForType(resty))
			resty = lex::I64;
		l->setLexeme(lex::Lexeme(loc, resty, res));
		*source = l;
		break;
	}
	case lex::MOD: {
		if(!l->getLexValue().isInt() || !r->getLexValue().isInt()) break;

		lex::TokType resty = lex::I32;
		int64_t res = l->getLexValue().getDataAsInt() % r->getLexValue().getDataAsInt();
		if(res > lex::getIntMaxForType(resty) || res < lex::getIntMinForType(resty))
			resty = lex::I64;
		l->setLexeme(lex::Lexeme(loc, resty, res));
		*source = l;
		break;
	}
	case lex::ADD_ASSN:
	case lex::SUB_ASSN:
	case lex::MUL_ASSN:
	case lex::DIV_ASSN:
	case lex::MOD_ASSN:
	// Post/Pre Inc/Dec
	case lex::XINC:
	case lex::INCX:
	case lex::XDEC:
	case lex::DECX: break;
	// Unary
	case lex::UADD: {
		if(!l->getLexValue().isNumeric()) break;
		if(l->getLexValue().isInt())
			l->setLexeme(lex::Lexeme(loc, l->getLexValue().getType(),
						 +l->getLexValue().getDataInt()));
		if(l->getLexValue().isFlt())
			l->setLexeme(lex::Lexeme(loc, l->getLexValue().getType(),
						 +l->getLexValue().getDataFlt()));
		*source = l;
		break;
	}
	case lex::USUB: {
		if(!l->getLexValue().isNumeric()) break;
		if(l->getLexValue().isInt())
			l->setLexeme(lex::Lexeme(loc, l->getLexValue().getType(),
						 -l->getLexValue().getDataInt()));
		if(l->getLexValue().isFlt())
			l->setLexeme(lex::Lexeme(loc, l->getLexValue().getType(),
						 -l->getLexValue().getDataFlt()));
		*source = l;
		break;
	}
	// Logic
	case lex::LAND: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			int64_t res =
			l->getLexValue().getDataAsFlt() && r->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = l->getLexValue().getDataAsInt() && r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::LOR: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			int64_t res =
			l->getLexValue().getDataAsFlt() || r->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = l->getLexValue().getDataAsInt() || r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::LNOT: {
		if(!l->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt()) {
			int64_t res = !l->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = !l->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	// Comparison
	case lex::EQ: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			int64_t res =
			l->getLexValue().getDataAsFlt() == r->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = l->getLexValue().getDataAsInt() == r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::LT: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			int64_t res =
			l->getLexValue().getDataAsFlt() < r->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = l->getLexValue().getDataAsInt() < r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::GT: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			int64_t res =
			l->getLexValue().getDataAsFlt() > r->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = l->getLexValue().getDataAsInt() > r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::LE: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			int64_t res =
			l->getLexValue().getDataAsFlt() <= r->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = l->getLexValue().getDataAsInt() <= r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::GE: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			int64_t res =
			l->getLexValue().getDataAsFlt() >= r->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = l->getLexValue().getDataAsInt() >= r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::NE: {
		if(!l->getLexValue().isNumeric() || !r->getLexValue().isNumeric()) break;

		if(l->getLexValue().isFlt() || r->getLexValue().isFlt()) {
			int64_t res =
			l->getLexValue().getDataAsFlt() != r->getLexValue().getDataAsFlt();
			l->setLexeme(lex::Lexeme(loc, lex::I1, res));
			*source = l;
			break;
		}
		int64_t res = l->getLexValue().getDataAsInt() != r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	// Bitwise
	case lex::BAND: {
		if(!l->getLexValue().isInt() || !r->getLexValue().isInt()) break;

		int64_t res = l->getLexValue().getDataAsInt() & r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::BOR: {
		if(!l->getLexValue().isInt() || !r->getLexValue().isInt()) break;

		int64_t res = l->getLexValue().getDataAsInt() | r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::BNOT: {
		if(!l->getLexValue().isInt()) break;

		int64_t res = ~l->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::BXOR: {
		if(!l->getLexValue().isInt() || !r->getLexValue().isInt()) break;

		int64_t res = l->getLexValue().getDataAsInt() ^ r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::BAND_ASSN:
	case lex::BOR_ASSN:
	case lex::BNOT_ASSN:
	case lex::BXOR_ASSN: break;
	// Others
	case lex::LSHIFT: {
		if(!l->getLexValue().isInt() || !r->getLexValue().isInt()) break;

		int64_t res = l->getLexValue().getDataAsInt() << r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::RSHIFT: {
		if(!l->getLexValue().isInt() || !r->getLexValue().isInt()) break;

		int64_t res = l->getLexValue().getDataAsInt() >> r->getLexValue().getDataAsInt();
		l->setLexeme(lex::Lexeme(loc, lex::I1, res));
		*source = l;
		break;
	}
	case lex::LSHIFT_ASSN:
	case lex::RSHIFT_ASSN: break;
	default: err::out(stmt->getOper(), "nonexistent operator"); return false;
	}
	return true;
}
bool ConstFoldPass::visit(StmtVar *stmt, Stmt **source)
{
	if(stmt->getVVal()) return visit(stmt->getVVal(), &stmt->getVVal());
	return true;
}
bool ConstFoldPass::visit(StmtFnSig *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtFnDef *stmt, Stmt **source)
{
	return visit(stmt->getBlk(), asStmt(&stmt->getBlk()));
}
bool ConstFoldPass::visit(StmtHeader *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtLib *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtExtern *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtEnum *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtStruct *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(auto &d : stmt->getDecls()) {
		if(!visit(d, asStmt(&d))) return false;
	}
	return true;
}
bool ConstFoldPass::visit(StmtCond *stmt, Stmt **source)
{
	for(auto &c : stmt->getConditionals()) {
		if(c.getCond() && !visit(c.getCond(), &c.getCond())) return false;
		if(c.getBlk() && !visit(c.getBlk(), asStmt(&c.getBlk()))) return false;
	}
	return true;
}
bool ConstFoldPass::visit(StmtFor *stmt, Stmt **source)
{
	if(stmt->getInit() && !visit(stmt->getInit(), &stmt->getInit())) return false;
	if(stmt->getCond() && !visit(stmt->getCond(), &stmt->getCond())) return false;
	if(stmt->getIncr() && !visit(stmt->getIncr(), &stmt->getIncr())) return false;
	if(stmt->getBlk() && !visit(stmt->getBlk(), asStmt(&stmt->getBlk()))) return false;
	return true;
}
bool ConstFoldPass::visit(StmtRet *stmt, Stmt **source)
{
	if(stmt->getRetVal() && !visit(stmt->getRetVal(), &stmt->getRetVal())) return false;
	return true;
}
bool ConstFoldPass::visit(StmtContinue *stmt, Stmt **source) { return true; }
bool ConstFoldPass::visit(StmtBreak *stmt, Stmt **source) { return true; }
} // namespace sc::AST