/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "CodeGen/C.hpp"

#include <cassert>

#include "CodeGen/C/Prelude.hpp"
#include "Parser.hpp"
#include "Utils.hpp"

namespace sc
{
CDriver::CDriver(RAIIParser &parser)
	: CodeGenDriver(parser), headers(default_includes), typedefs(default_typedefs)
{}
CDriver::~CDriver() {}

bool CDriver::compile(const std::string &outfile, const bool &ir_only)
{
	Module *mainmod = parser.getModule(parser.getModuleStack().front());
	Writer mainwriter;
	if(!visit(mainmod->getParseTree(), mainwriter, false)) {
		err.set(mainmod->getParseTree(), "failed to compile module: %s",
			mainmod->getPath().c_str());
		err.show(stderr);
		return false;
	}
	Writer finalmod;
	for(auto &h : headers) {
		finalmod.write("#include " + h);
		finalmod.newLine();
	}
	if(headers.size() > 0) finalmod.newLine();
	for(auto &m : macros) {
		finalmod.write(m);
		finalmod.newLine();
	}
	if(macros.size() > 0) finalmod.newLine();
	for(auto &t : typedefs) {
		finalmod.write(t);
		finalmod.newLine();
	}
	if(typedefs.size() > 0) finalmod.newLine();
	for(auto &d : structdecls) {
		finalmod.write(d);
		finalmod.newLine();
	}
	if(structdecls.size() > 0) finalmod.newLine();
	for(auto &d : funcdecls) {
		finalmod.write(d);
		finalmod.newLine();
	}
	if(funcdecls.size() > 0) finalmod.newLine();
	for(auto &c : constants) {
		finalmod.write(c.second.decl);
		finalmod.newLine();
	}
	if(constants.size() > 0) finalmod.newLine();
	finalmod.append(mainwriter);
	if(ir_only) {
		FILE *f = fopen(outfile.c_str(), "w+");
		fprintf(f, "%s\n", finalmod.getData().c_str());
		fclose(f);
		return true;
	}
	// TODO: full compilation
	return true;
}

bool CDriver::visit(Stmt *stmt, Writer &writer, const bool &semicol)
{
	bool res = false;
	Writer tmp(writer);
	switch(stmt->getStmtType()) {
	case BLOCK: res = visit(as<StmtBlock>(stmt), tmp, semicol); break;
	case TYPE: res = visit(as<StmtType>(stmt), tmp, semicol); break;
	case SIMPLE: res = visit(as<StmtSimple>(stmt), tmp, semicol); break;
	case EXPR: res = visit(as<StmtExpr>(stmt), tmp, semicol); break;
	case FNCALLINFO: res = visit(as<StmtFnCallInfo>(stmt), tmp, semicol); break;
	case VAR: res = visit(as<StmtVar>(stmt), tmp, semicol); break;
	case FNSIG: res = visit(as<StmtFnSig>(stmt), tmp, semicol); break;
	case FNDEF: res = visit(as<StmtFnDef>(stmt), tmp, semicol); break;
	case HEADER: res = visit(as<StmtHeader>(stmt), tmp, semicol); break;
	case LIB: res = visit(as<StmtLib>(stmt), tmp, semicol); break;
	case EXTERN: res = visit(as<StmtExtern>(stmt), tmp, semicol); break;
	case ENUMDEF: res = visit(as<StmtEnum>(stmt), tmp, semicol); break;
	case STRUCTDEF: res = visit(as<StmtStruct>(stmt), tmp, semicol); break;
	case VARDECL: res = visit(as<StmtVarDecl>(stmt), tmp, semicol); break;
	case COND: res = visit(as<StmtCond>(stmt), tmp, semicol); break;
	case FORIN: res = visit(as<StmtForIn>(stmt), tmp, semicol); break;
	case FOR: res = visit(as<StmtFor>(stmt), tmp, semicol); break;
	case WHILE: res = visit(as<StmtWhile>(stmt), tmp, semicol); break;
	case RET: res = visit(as<StmtRet>(stmt), tmp, semicol); break;
	case CONTINUE: res = visit(as<StmtContinue>(stmt), tmp, semicol); break;
	case BREAK: res = visit(as<StmtBreak>(stmt), tmp, semicol); break;
	case DEFER: res = visit(as<StmtDefer>(stmt), tmp, semicol); break;
	default: {
		err.set(stmt, "invalid statement found for C code generation: %s",
			stmt->getStmtTypeCString());
		break;
	}
	}
	if(!res) return false;

	if(stmt->hasCast()) {
		writer.write("(");
		writer.write(getCTypeName(stmt, stmt->getType(), true));
		writer.write(")(");
		writer.append(tmp);
		writer.write(")");
	} else {
		writer.append(tmp);
	}
	return res;
}

bool CDriver::visit(StmtBlock *stmt, Writer &writer, const bool &semicol)
{
	if(!stmt->isTop()) {
		writer.write("{");
		writer.addIndent();
		writer.newLine();
	}
	for(size_t i = 0; i < stmt->getStmts().size(); ++i) {
		auto &s = stmt->getStmts()[i];
		Writer tmp(writer);
		if(!visit(s, tmp, acceptsSemicolon(s))) {
			err.set(stmt, "failed to generate IR for block");
			return false;
		}
		writer.append(tmp);
		if(i < stmt->getStmts().size() - 1) writer.newLine();
	}
	if(!stmt->isTop()) {
		writer.remIndent();
		writer.newLine();
		writer.write("}");
	}
	if(semicol) writer.write(";");
	return true;
}
bool CDriver::visit(StmtType *stmt, Writer &writer, const bool &semicol)
{
	writer.clear();
	return false;
}
bool CDriver::visit(StmtSimple *stmt, Writer &writer, const bool &semicol)
{
	writer.clear();
	if(stmt->getValue()) {
		writer.write(getCValue(stmt, stmt->getValue(), stmt->getType()));
		return true;
	}
	switch(stmt->getLexValue().getTok().getVal()) {
	case lex::TRUE:	 // fallthrough
	case lex::FALSE: // fallthrough
	case lex::NIL:	 // fallthrough
	case lex::INT:	 // fallthrough
	case lex::FLT:	 // fallthrough
	case lex::CHAR:	 // fallthrough
	case lex::STR: writer.write(getConstantDataVar(stmt->getLexValue())); return true;
	default: break;
	}
	// the following part is only valid for existing variables.
	// the part for variable declaration exists in Var visit
	if(stmt->getType()->hasRef()) writer.write("(*"); // for references
	writer.write(getMangledName(stmt->getLexValue().getDataStr(), stmt));
	if(stmt->getType()->hasRef()) writer.write(")"); // for references
	if(semicol) writer.write(";");
	return true;
}
bool CDriver::visit(StmtFnCallInfo *stmt, Writer &writer, const bool &semicol)
{
	return true;
}
bool CDriver::visit(StmtExpr *stmt, Writer &writer, const bool &semicol)
{
	writer.clear();
	if(stmt->getValue()) {
		writer.write(getCValue(stmt, stmt->getValue(), stmt->getType()));
		if(semicol) writer.write(";");
		return true;
	}

	lex::TokType oper = stmt->getOper().getTok().getVal();
	Writer l;
	Stmt *&lhs = stmt->getLHS();
	Stmt *&rhs = stmt->getRHS();
	if(!visit(lhs, l, false)) {
		err.set(stmt, "failed to generate C code for LHS in expression");
		return false;
	}
	Writer r;
	if(oper != lex::DOT && oper != lex::ARROW && oper != lex::FNCALL && rhs &&
	   !visit(rhs, r, false)) {
		rhs->disp(false);
		err.set(stmt, "failed to generate C code for RHS in expression");
		return false;
	}
	switch(oper) {
	case lex::ARROW:
	case lex::DOT: {
		StmtSimple *rsim = as<StmtSimple>(rhs);
		writer.append(l);
		writer.write(".");
		writer.write(rsim->getLexValue().getDataStr());
		break;
	}
	case lex::FNCALL: {
		std::vector<Stmt *> &args = as<StmtFnCallInfo>(rhs)->getArgs();
		if(lhs->getType()->isFunc()) {
			writer.write("func_%" PRIu64 "(", lhs->getType()->getID());
			FuncTy *fn = as<FuncTy>(lhs->getType());
			for(size_t i = 0; i < args.size(); ++i) {
				Stmt *&a = args[i];
				Type *at = fn->getArg(i);
				Writer tmp(writer);
				if(!visit(args[i], tmp, false)) {
					err.set(stmt,
						"failed to generate C code for func call argument");
					return false;
				}
				if(at->hasRef()) {
					writer.write("&(");
					writer.append(tmp);
					writer.write(")");
				} else {
					writer.append(tmp);
				}
				if(i != args.size() - 1) writer.write(", ");
			}
			writer.write(")");
		} else {
			writer.write("struct_%" PRIu64 "{", lhs->getType()->getID());
			StructTy *st = as<StructTy>(lhs->getType());
			for(size_t i = 0; i < args.size(); ++i) {
				Stmt *&a = args[i];
				Type *at = st->getField(i);
				Writer tmp(writer);
				if(!visit(args[i], tmp, false)) {
					err.set(stmt, "failed to generate C code"
						      " for struct declaration argument");
					return false;
				}
				if(at->hasRef()) {
					writer.write("&(");
					writer.append(tmp);
					writer.write(")");
				} else {
					writer.append(tmp);
				}
				if(i != args.size() - 1) writer.write(", ");
			}
			writer.write("}");
		}
		break;
	}
	// address of
	case lex::UAND: {
		writer.write("&");
		writer.append(l);
		break;
	}
	// dereference
	case lex::UMUL: {
		writer.write("*");
		writer.append(l);
		break;
	}
	case lex::SUBS:
	case lex::ASSN:
	// Arithmetic
	case lex::ADD:
	case lex::SUB:
	case lex::MUL:
	case lex::DIV:
	case lex::MOD:
	case lex::ADD_ASSN:
	case lex::SUB_ASSN:
	case lex::MUL_ASSN:
	case lex::DIV_ASSN:
	case lex::MOD_ASSN:
	// Post/Pre Inc/Dec
	case lex::XINC:
	case lex::INCX:
	case lex::XDEC:
	case lex::DECX:
	// Unary
	case lex::UADD:
	case lex::USUB:
	// Logic
	case lex::LAND:
	case lex::LOR:
	case lex::LNOT:
	// Comparison
	case lex::EQ:
	case lex::LT:
	case lex::GT:
	case lex::LE:
	case lex::GE:
	case lex::NE:
	// Bitwise
	case lex::BAND:
	case lex::BOR:
	case lex::BNOT:
	case lex::BXOR:
	case lex::BAND_ASSN:
	case lex::BOR_ASSN:
	case lex::BNOT_ASSN:
	case lex::BXOR_ASSN:
	// Others
	case lex::LSHIFT:
	case lex::RSHIFT:
	case lex::LSHIFT_ASSN:
	case lex::RSHIFT_ASSN: {
	applyoperfn:
		lex::Tok &optok = stmt->getOper().getTok();
		if(oper == lex::SUBS && lhs->getType()->isPtr()) {
			writer.append(l);
			writer.write("[");
			writer.append(r);
			writer.write("]");
			break;
		}
		if(lhs->getType()->isPrimitive() && (!rhs || rhs->getType()->isPrimitive())) {
			if(optok.isUnaryPre()) {
				writer.write(optok.getUnaryNoCharCStr());
				writer.append(l);
				break;
			}
			if(optok.isUnaryPost()) {
				writer.append(l);
				writer.write(optok.getUnaryNoCharCStr());
				break;
			}
			writer.append(l);
			writer.write(" %s ", lex::TokStrs[oper]);
			writer.append(r);
			break;
		}
		writer.write(optok.getOperCStr());
		writer.write("(");
		writer.append(l);
		if(rhs) {
			writer.write(", ");
			writer.append(r);
		}
		writer.write(")");
		break;
	}
	default: err.set(stmt->getOper(), "nonexistent operator"); return false;
	}
	if(semicol) writer.write(";");
	return true;
}
bool CDriver::visit(StmtVar *stmt, Writer &writer, const bool &semicol)
{
	writer.clear();
	std::string varname = getMangledName(stmt->getName().getDataStr(), stmt);

	if(stmt->getValue()) {
		std::string type = getCTypeName(stmt, stmt->getType(), false);
		writer.write("%s %s = %s", type.c_str(), varname.c_str(),
			     getCValue(stmt, stmt->getValue(), stmt->getType()).c_str());
		if(semicol) writer.write(";");
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->getStmtType() == EXTERN) {
		StmtExtern *ext	  = as<StmtExtern>(stmt->getVVal());
		size_t args	  = ext->getSigArgs().size();
		std::string macro = "#define " + varname + "(";
		std::string argstr;
		for(size_t i = 0; i < args; ++i) {
			argstr += 'a' + i;
			argstr += ", ";
		}
		if(args) {
			argstr.pop_back();
			argstr.pop_back();
		}
		macro += argstr + ") ";
		macro += ext->getFnName().getDataStr() + "(" + argstr + ")";
		macros.push_back(macro);
		if(!visit(stmt->getVVal(), writer, false)) {
			err.set(stmt, "failed to generate C code for extern variable");
			return false;
		}
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->getStmtType() == FNDEF) {
		Writer tmp(writer);
		if(!visit(stmt->getVVal(), tmp, false)) {
			err.set(stmt, "failed to generate C code for function def");
			return false;
		}
		// set as entry point (main function) if signature matches
		static bool maindone = false;
		if(!maindone && trySetMainFunction(stmt, varname, writer)) {
			maindone = true;
			varname	 = "main";
		}

		StmtFnDef *fn = as<StmtFnDef>(stmt->getVVal());
		std::string ret =
		getCTypeName(fn->getSigRetType(), fn->getSigRetType()->getType(), true);

		tmp.insertAfter(ret.size(), " " + varname);
		writer.append(tmp);
		// no semicolon after fndef

		// add declaration (at the top) for the function
		Writer decl;
		if(!visit(as<StmtFnDef>(stmt->getVVal())->getSig(), decl, true)) {
			err.set(stmt, "failed to generate C code for function def");
			return false;
		}
		decl.insertAfter(ret.size(), " " + varname);
		funcdecls.push_back(decl.getData());
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->getStmtType() == STRUCTDEF) {
		// structs are not defined by themselves
		// they are defined when a struct type is encountered
		return true;
	}

	Writer tmp(writer);
	if(stmt->getVVal() && !visit(stmt->getVVal(), tmp, false)) {
		err.set(stmt, "failed to generate C code from scribe declaration value");
		return false;
	}
	std::string type = getCTypeName(stmt, stmt->getType(), false);
	if(type.empty()) {
		err.set(stmt, "no C type found for the variable type: %s",
			stmt->getType()->toStr().c_str());
		return false;
	}
	writer.write("%s %s", type.c_str(), varname.c_str());
	if(!tmp.empty()) {
		writer.write(" = ");
		if(stmt->getType()->hasRef()) {
			writer.write("&(");
			writer.append(tmp);
			writer.write(")");
		} else {
			writer.append(tmp);
		}
	}
	if(semicol) writer.write(";");
	return true;
}
bool CDriver::visit(StmtFnSig *stmt, Writer &writer, const bool &semicol)
{
	writer.write(getCTypeName(stmt->getRetType(), stmt->getRetType()->getType(), true));
	writer.write("(");
	for(size_t i = 0; i < stmt->getArgs().size(); ++i) {
		auto &a = stmt->getArgs()[i];
		Writer tmp(writer);
		if(!visit(a, tmp, false)) {
			err.set(stmt, "failed to generate C code for function arg");
			return false;
		}
		writer.append(tmp);
		if(i < stmt->getArgs().size() - 1) writer.write(", ");
	}
	writer.write(")");
	if(semicol) writer.write(";");
	return true;
}
bool CDriver::visit(StmtFnDef *stmt, Writer &writer, const bool &semicol)
{
	if(!visit(stmt->getSig(), writer, false)) {
		err.set(stmt, "failed to generate C code for function");
		return false;
	}
	if(!stmt->getBlk()) return true;
	writer.write(" ");
	if(!visit(stmt->getBlk(), writer, false)) {
		err.set(stmt, "failed to generate C code for function block");
		return false;
	}
	return true;
}
bool CDriver::visit(StmtHeader *stmt, Writer &writer, const bool &semicol)
{
	if(!stmt->getNames().getDataStr().empty()) {
		std::vector<std::string> headersf = stringDelim(stmt->getNames().getDataStr(), ",");
		for(auto &h : headersf) {
			if(isOneOf(headers, h)) continue;
			headers.push_back(h);
		}
	}
	if(!stmt->getFlags().getDataStr().empty()) {
		headerflags.push_back(stmt->getFlags().getDataStr());
	}
	return true;
}
bool CDriver::visit(StmtLib *stmt, Writer &writer, const bool &semicol)
{
	if(!stmt->getFlags().getDataStr().empty()) {
		libflags.push_back(stmt->getFlags().getDataStr());
	}
	return true;
}
bool CDriver::visit(StmtExtern *stmt, Writer &writer, const bool &semicol)
{
	if(stmt->getHeaders()) visit(stmt->getHeaders(), writer, false);
	if(stmt->getLibs()) visit(stmt->getLibs(), writer, false);
	// nothing to do of signature
	return true;
}
bool CDriver::visit(StmtEnum *stmt, Writer &writer, const bool &semicol)
{
	err.set(stmt, "Unimplemented enum C code generation");
	return false;
}
bool CDriver::visit(StmtStruct *stmt, Writer &writer, const bool &semicol)
{
	// structs are not defined by themselves
	// they are defined when a struct type is encountered
	return true;
}
bool CDriver::visit(StmtVarDecl *stmt, Writer &writer, const bool &semicol)
{
	for(auto &d : stmt->getDecls()) {
		Writer tmp(writer);
		if(!visit(d, tmp, true)) {
			err.set(stmt, "failed to generate C code for var decl");
			return false;
		}
		writer.append(tmp);
	}
	return true;
}
bool CDriver::visit(StmtCond *stmt, Writer &writer, const bool &semicol)
{
	if(stmt->isInline()) {
		if(stmt->getConditionals().empty()) return true;
		if(!visit(stmt->getConditionals().back().getBlk(), writer, false)) {
			err.set(stmt, "failed to generate C code for inline conditional block");
			return false;
		}
		return true;
	}
	for(size_t i = 0; i < stmt->getConditionals().size(); ++i) {
		auto &c = stmt->getConditionals()[i];
		if(i > 0) writer.write(" else ");
		if(c.getCond()) {
			writer.write("if(");
			Writer tmp(writer);
			if(!visit(c.getCond(), tmp, false)) {
				err.set(c.getCond(), "failed to generate C code for conditional");
				return false;
			}
			writer.append(tmp);
			writer.write(") ");
		}
		if(!visit(c.getBlk(), writer, false)) {
			err.set(stmt, "failed to generate C code for conditional block");
			return false;
		}
	}
	return true;
}
bool CDriver::visit(StmtForIn *stmt, Writer &writer, const bool &semicol)
{
	err.set(stmt, "Unimplemented for-in C code generation");
	return false;
}
bool CDriver::visit(StmtFor *stmt, Writer &writer, const bool &semicol)
{
	if(stmt->isInline()) {
		if(stmt->getBlk()->getStmts().empty()) return true;
		Writer tmp(writer);
		if(!visit(stmt->getBlk(), tmp, false)) {
			err.set(stmt, "failed to generate C code for inline for-loop block");
			return false;
		}
		writer.append(tmp);
		return true;
	}
	writer.write("for(");
	if(stmt->getInit()) {
		Writer tmp(writer);
		if(!visit(stmt->getInit(), tmp, false)) {
			err.set(stmt, "failed to generate C code for for-loop init");
			return false;
		}
		writer.append(tmp);
	}
	writer.write(";");
	if(stmt->getCond()) {
		writer.write(" ");
		Writer tmp(writer);
		if(!visit(stmt->getCond(), tmp, false)) {
			err.set(stmt, "failed to generate C code for for-loop condition");
			return false;
		}
		writer.append(tmp);
	}
	writer.write(";");
	if(stmt->getIncr()) {
		writer.write(" ");
		Writer tmp(writer);
		if(!visit(stmt->getIncr(), tmp, false)) {
			err.set(stmt, "failed to generate C code for for-loop incr");
			return false;
		}
		writer.append(tmp);
	}
	writer.write(") ");
	Writer tmp(writer);
	if(!visit(stmt->getBlk(), tmp, false)) {
		err.set(stmt, "failed to generate C code for for-loop block");
		return false;
	}
	writer.append(tmp);
	return true;
}
bool CDriver::visit(StmtWhile *stmt, Writer &writer, const bool &semicol)
{
	writer.clear();
	writer.write("while(");
	Writer tmp(writer);
	if(!visit(stmt->getCond(), tmp, false)) {
		err.set(stmt, "failed to generate C code for while-loop condition");
		return false;
	}
	writer.append(tmp);
	writer.write(") ");
	tmp.reset(writer);
	if(!visit(stmt->getBlk(), tmp, false)) {
		err.set(stmt, "failed to generate C code for while block");
		return false;
	}
	writer.append(tmp);
	return true;
}
bool CDriver::visit(StmtRet *stmt, Writer &writer, const bool &semicol)
{
	if(!stmt->getVal()) {
		writer.write("return");
		if(semicol) writer.write(";");
		return true;
	}
	Writer tmp(writer);
	if(!visit(stmt->getVal(), tmp, false)) {
		err.set(stmt, "failed to generate C code for return value");
		return false;
	}
	writer.write("return ");
	writer.append(tmp);
	if(semicol) writer.write(";");
	return true;
}
bool CDriver::visit(StmtContinue *stmt, Writer &writer, const bool &semicol)
{
	writer.write("continue");
	if(semicol) writer.write(";");
	return true;
}
bool CDriver::visit(StmtBreak *stmt, Writer &writer, const bool &semicol)
{
	writer.write("break");
	if(semicol) writer.write(";");
	return true;
}
bool CDriver::visit(StmtDefer *stmt, Writer &writer, const bool &semicol)
{
	err.set(stmt, "defer should never come as a part of code generation");
	return false;
}

const std::string &CDriver::getConstantDataVar(const lex::Lexeme &val)
{
	std::string key;
	std::string value;
	std::string type;
	switch(val.getTok().getVal()) {
	case lex::TRUE:
		value = "1";
		key   = value + "i1";
		type  = "const i1";
		break;
	case lex::FALSE:
		value = "0";
		key   = value + "i1";
		type  = "const i1";
		break;
	case lex::NIL:
		value = "0";
		key   = value + "i1";
		type  = "const i1";
		break;
	case lex::INT:
		value = std::to_string(val.getDataInt());
		key   = value + "i32";
		type  = "const i32";
		break;
	case lex::FLT:
		value = std::to_string(val.getDataFlt());
		key   = value + "f32";
		type  = "const f32";
		break;
	case lex::CHAR:
		value = '\'' + std::to_string(val.getDataStr()[0]) + '\'';
		key   = value;
		type  = "const i8";
		break;
	case lex::STR:
		value = '"' + val.getDataStr() + '"';
		key   = value;
		type  = "const i8*";
		break;
	default: break;
	}
	if(key.empty()) {
		value = "0";
		key   = "0i32";
		type  = "const i32";
	}

	auto res = constants.find(key);
	if(res != constants.end()) return res->second.var;
	std::string var	 = getNewConstantVar();
	std::string decl = type + " " + var + " = " + value + ";";
	constants[key]	 = {var, decl};
	return constants[key].var;
}
std::string CDriver::getNewConstantVar()
{
	static size_t const_id = 0;
	return "const_" + std::to_string(const_id++);
}

bool CDriver::acceptsSemicolon(Stmt *stmt)
{
	switch(stmt->getStmtType()) {
	case BLOCK: return false;
	case TYPE: return false;
	case SIMPLE: return true;
	case EXPR: return true;
	case FNCALLINFO: return false;
	case VAR: return true;
	case FNSIG: return false;
	case FNDEF: return false;
	case HEADER: return false;
	case LIB: return false;
	case EXTERN: return false;
	case ENUMDEF: return false;
	case STRUCTDEF: return false;
	case VARDECL: return false;
	case COND: return false;
	case FORIN: return false;
	case FOR: return false;
	case WHILE: return false;
	case RET: return true;
	case CONTINUE: return true;
	case BREAK: return true;
	case DEFER: return true;
	}
	return false;
}

bool CDriver::trySetMainFunction(StmtVar *var, const std::string &varname, Writer &writer)
{
	StmtFnDef *fn = as<StmtFnDef>(var->getVVal());
	if(!startsWith(var->getName().getDataStr(), "main_0")) return false;
	Type *retbase = fn->getSig()->getRetType()->getType();
	if(!retbase || !retbase->isInt()) return false;
	// false => 0 args
	// true => 2 args
	bool zero_or_two = false;
	if(fn->getSigArgs().empty()) { // int main()
		return true;
	} else if(fn->getSigArgs().size() == 2) { // int main(int argc, char **argv)
		Type *a1 = fn->getSigArgs()[0]->getType();
		Type *a2 = fn->getSigArgs()[0]->getType();
		if(!a1->isInt()) return false;
		if(!a2->isPtr()) return false;
		if(!as<PtrTy>(a2)->getTo()->isPtr()) return false;
		if(!as<PtrTy>(as<PtrTy>(a2)->getTo())->getTo()->isInt()) return false;
		return true;
	}
	return false;
}

std::string CDriver::getCTypeName(Stmt *stmt, Type *ty, bool arr_as_ptr)
{
	std::string pre;
	std::string post;
	if(ty->hasStatic()) pre += "static ";
	if(ty->hasConst()) pre += "const ";
	if(ty->hasVolatile()) pre += "volatile ";
	if(ty->hasRef()) post += " *";

	if(ty->isVoid()) {
		return pre + "void" + post;
	}
	if(ty->isTypeTy()) {
		Type *cty = as<TypeTy>(ty)->getContainedTy();
		return pre + getCTypeName(stmt, cty, arr_as_ptr) + post;
	}
	if(ty->isInt()) {
		bool is_signed	 = as<IntTy>(ty)->isSigned();
		std::string bits = std::to_string(as<IntTy>(ty)->getBits());
		return pre + (is_signed ? "i" : "u") + bits + post;
	}
	if(ty->isFlt()) {
		std::string bits = std::to_string(as<FltTy>(ty)->getBits());
		return pre + "f" + bits + post;
	}
	if(ty->isPtr()) {
		Type *to	= as<PtrTy>(ty)->getTo();
		std::string res = getCTypeName(stmt, to, arr_as_ptr);
		if(arr_as_ptr || as<PtrTy>(ty)->getCount() == 0) {
			return res + "*";
		}
		std::string arrcount = "[" + std::to_string(as<PtrTy>(ty)->getCount()) + "]";
		return pre + res + arrcount + post;
	}
	if(ty->isFunc()) {
		FuncTy *f = as<FuncTy>(ty);
		std::string res;
		res += getCTypeName(stmt, f->getRet(), true);
		res += "(*func_" + std::to_string(f->getID()) + ")";
		res += "(";
		for(auto &t : f->getArgs()) {
			res += getCTypeName(stmt, t, true);
			res += ", ";
		}
		if(f->getArgs().size() > 0) {
			res.pop_back();
			res.pop_back();
		}
		res += ")";
		return res;
	}
	if(ty->isStruct()) {
		StructTy *s = as<StructTy>(ty);
		addStructDef(stmt, s);
		std::string res = "struct_" + std::to_string(s->getID());
		return pre + res + post;
	}
	return "";
}
std::string CDriver::getCValue(Stmt *stmt, Value *value, Type *type)
{
	switch(value->getType()) {
	case VVOID: return "";
	case VINT: {
		IntTy *t = as<IntTy>(type);
		if(t->getBits() == 8 && t->isSigned()) {
			return "'" + std::string(1, as<IntVal>(value)->getVal()) + "'";
		}
		return std::to_string(as<IntVal>(value)->getVal());
	}
	case VFLT: return std::to_string(as<FltVal>(value)->getVal());
	case VVEC: {
		bool is_str = false;
		printf("hi there %s\n", type->toStr().c_str());
		assert(type->isPtr());
		if(as<PtrTy>(type)->getTo()->isInt()) {
			IntTy *t = as<IntTy>(as<PtrTy>(type)->getTo());
			if(t->getBits() == 8 && t->isSigned()) is_str = true;
		}
		if(is_str) {
			return "\"" + as<VecVal>(value)->getAsString() + "\"";
		}
		std::string res;
		res	 = "{";
		Type *to = as<PtrTy>(type)->getTo();
		for(auto &e : as<VecVal>(value)->getVal()) {
			res += getCValue(stmt, e, to) + ", ";
		}
		if(as<VecVal>(value)->getVal().size() > 0) {
			res.pop_back();
			res.pop_back();
		}
		res += "}";
		return res;
	}
	case VSTRUCT: {
		std::string res;
		StructTy *st = as<StructTy>(type);

		res = "{";
		for(size_t i = 0; i < st->getFields().size(); ++i) {
			Value *fv = as<StructVal>(value)->getValAttr(st->getFieldName(i));
			res += getCValue(stmt, fv, st->getField(i));
		}
		if(st->getFields().size() > 0) {
			res.pop_back();
			res.pop_back();
		}
		res += "}";
		return res;
	}
	}
	return "";
}
void CDriver::addStructDef(Stmt *stmt, StructTy *sty)
{
	static std::unordered_set<uint64_t> declaredstructs;
	if(declaredstructs.find(sty->getID()) != declaredstructs.end()) return;
	Writer st;
	st.write("typedef struct_%" PRIu64 " struct {", sty->getID());
	if(!sty->getFields().empty()) {
		st.addIndent();
		st.newLine();
	}
	for(size_t i = 0; i < sty->getFields().size(); ++i) {
		st.write(getCTypeName(stmt, sty->getField(i), false));
		st.write(" %s;", sty->getFieldName(i).c_str());
		if(i != sty->getFields().size() - 1) st.newLine();
	}
	if(!sty->getFields().empty()) {
		st.remIndent();
		st.newLine();
	}
	st.write("};");
	structdecls.push_back(st.getData());
	declaredstructs.insert(sty->getID());
}
} // namespace sc