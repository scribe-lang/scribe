#include "AST/Passes/IRGen.hpp"

#include "VM/Instructions.hpp"

namespace sc::ast
{

IRGenPass::IRGenPass(IRBuilder &b) : Pass(Pass::genPassID<IRGenPass>()), b(b) {}
IRGenPass::~IRGenPass() {}

bool IRGenPass::visit(Stmt *stmt, Stmt **source)
{
	bool res = false;
	switch(stmt->getStmtType()) {
	case BLOCK: res = visit(as<StmtBlock>(stmt), source); break;
	case TYPE: res = visit(as<StmtType>(stmt), source); break;
	case SIMPLE: res = visit(as<StmtSimple>(stmt), source); break;
	case EXPR: res = visit(as<StmtExpr>(stmt), source); break;
	case CALLARGS: res = visit(as<StmtCallArgs>(stmt), source); break;
	case VAR: res = visit(as<StmtVar>(stmt), source); break;
	case SIGNATURE: res = visit(as<StmtSignature>(stmt), source); break;
	case FNDEF: res = visit(as<StmtFnDef>(stmt), source); break;
	case VARDECL: res = visit(as<StmtVarDecl>(stmt), source); break;
	case COND: res = visit(as<StmtCond>(stmt), source); break;
	case FOR: res = visit(as<StmtFor>(stmt), source); break;
	case ONEWORD: res = visit(as<StmtOneWord>(stmt), source); break;
	default: {
		err::out(stmt, "invalid statement found for IR gen: ", stmt->getStmtTypeCString());
		break;
	}
	}
	return res;
}

bool IRGenPass::visit(StmtBlock *stmt, Stmt **source)
{
	for(auto &s : stmt->getStmts()) {
		if(!visit(s, &s)) {
			err::out(stmt, "failed to generate IR for statement");
			return false;
		}
	}
	return true;
}
bool IRGenPass::visit(StmtType *stmt, Stmt **source)
{
	if(!visit(stmt->getExpr(), &stmt->getExpr())) {
		err::out(stmt, "failed to generate IR for type expr statement");
		return false;
	}
	b.getLastInst()->setAttributes(stmt->getAttributes());
	return true;
}
bool IRGenPass::visit(StmtSimple *stmt, Stmt **source)
{
	lex::Lexeme &l	     = stmt->getLexeme();
	ModuleLoc loc	     = stmt->getLoc();
	SimpleValue *val     = nullptr;
	Allocator &allocator = b.getAllocator();
	switch(l.getType()) {
	case lex::INT:
		val = SimpleValue::create(allocator, loc, SimpleValues::INT, l.getDataInt());
		break;
	case lex::FLT:
		val = SimpleValue::create(allocator, loc, SimpleValues::FLT, l.getDataFlt());
		break;
	case lex::STR:
		val = SimpleValue::create(allocator, loc, SimpleValues::STR, l.getDataStr());
		break;
	case lex::CHAR:
		val = SimpleValue::create(allocator, loc, SimpleValues::CHAR, l.getDataStr());
		break;
	case lex::IDEN:
		val = SimpleValue::create(allocator, loc, SimpleValues::IDEN, l.getDataStr());
		break;
	default:
		err::out(loc, "failed to generate IR for simple statement with data: ", l.str(0));
		return false;
	}

	b.addInst<LoadInstruction>(loc, val);

	return true;
}
bool IRGenPass::visit(StmtCallArgs *stmt, Stmt **source)
{
	for(auto &a : stmt->getArgs()) {
		if(!visit(a, &a)) {
			err::out(stmt, "failed to generate IR for call args statement");
			return false;
		}
	}
	return true;
}
bool IRGenPass::visit(StmtExpr *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtVar *stmt, Stmt **source)
{
	Stmt *&vval    = stmt->getVVal();
	StmtType *&vty = stmt->getVType();
	if(vval && !visit(vval, &vval)) {
		err::out(stmt, "failed to generate IR for var's value");
		return false;
	}
	if(vty && !visit(vty, asStmt(&vty))) {
		err::out(stmt, "failed to generate IR for var's type");
		return false;
	}
	StringRef name = stmt->getName().getDataStr();
	Instruction *i = b.addInst<CreateVarInstruction>(stmt->getLoc(), name);
	i->setAttributes(stmt->getAttributes());
	if(vval) i->addAttribute("value");
	if(vty) i->addAttribute("type");
	return true;
}
bool IRGenPass::visit(StmtSignature *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtFnDef *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtVarDecl *stmt, Stmt **source)
{
	for(auto &s : stmt->getDecls()) {
		if(!visit(s, asStmt(&s))) {
			err::out(stmt, "Failed to generate IR for var decl statement");
			return false;
		}
	}
	return true;
}
bool IRGenPass::visit(StmtCond *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtFor *stmt, Stmt **source) { return true; }
bool IRGenPass::visit(StmtOneWord *stmt, Stmt **source) { return true; }

} // namespace sc::ast