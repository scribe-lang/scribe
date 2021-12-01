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
#include "Env.hpp"
#include "FS.hpp"
#include "Parser.hpp"
#include "Utils.hpp"

namespace sc
{
CDriver::CDriver(RAIIParser &parser)
	: CodeGenDriver(parser), preheadermacros(default_preheadermacros),
	  headers(default_includes), typedefs(default_typedefs)
{}
CDriver::~CDriver() {}

bool CDriver::compile(const std::string &outfile)
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
	for(auto &ph : preheadermacros) {
		finalmod.write(ph);
		finalmod.newLine();
	}
	if(preheadermacros.size() > 0) finalmod.newLine();
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

	args::ArgParser &cliargs = parser.getCommandArgs();
	std::string opt		 = "0";
	std::string std		 = "11";
	bool ir_only		 = cliargs.has("ir");
	bool llir		 = cliargs.has("llir");
	if(cliargs.has("opt")) {
		std::string res = cliargs.val("opt");
		if(res.empty()) {
			err.set(mainmod->getParseTree(),
				"optimization option must have a value, found nothing");
			err.show(stderr);
			return false;
		}
		if(res != "0" && res != "1" && res != "2" && res != "3") {
			err.set(mainmod->getParseTree(), "optimization option value must be"
							 " one of 0, 1, 2, or 3; found nothing");
			err.show(stderr);
			return false;
		}
		opt = res;
	}
	if(cliargs.has("std")) {
		std::string res = cliargs.val("std");
		if(res.empty()) {
			err.set(mainmod->getParseTree(),
				"standard option must have a value, found nothing");
			err.show(stderr);
			return false;
		}
		if(res != "11" && res != "14" && res != "17") {
			err.set(mainmod->getParseTree(), "standard option value must be"
							 " one of 11, 14, or 17; found nothing");
			err.show(stderr);
			return false;
		}
		std = res;
	}

	std::string tmpfile;
	if(ir_only) {
		tmpfile = outfile + ".c";
	} else {
		auto loc = outfile.find_last_of('/');
		tmpfile	 = (loc == std::string::npos ? outfile : outfile.substr(loc));
		tmpfile	 = "/tmp/" + tmpfile + ".c";
	}
	FILE *f = fopen(tmpfile.c_str(), "w+");
	if(!f) {
		err.set(mainmod->getParseTree(), "failed to create file for writing C code: %s",
			tmpfile.c_str());
		err.show(stderr);
		return false;
	}
	fprintf(f, "%s\n", finalmod.getData().c_str());
	fclose(f);
	if(ir_only) return true;

	std::string cmd = getSystemCompiler();
	cmd += " -std=c" + std + " -O" + opt + " ";
	for(auto &h : headerflags) {
		cmd += h + " ";
	}
	for(auto &l : libflags) {
		cmd += l + " ";
	}
	cmd += tmpfile + " -o " + outfile;
	if(llir) cmd += ".ll -S -emit-llvm";
	int res = std::system(cmd.c_str());
	res	= WEXITSTATUS(res);
	if(res) {
		err.set(mainmod->getParseTree(),
			"failed to compile code, got compiler exit status: %d", res);
		err.show(stderr);
		return false;
	}
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
	if(tmp.empty()) return true;
	if(!semicol &&
	   (stmt->isExpr() ||
	    (stmt->isSimple() && as<StmtSimple>(stmt)->getLexValue().getTokVal() == lex::IDEN)) &&
	   stmt->getValueTy(true)->hasRef())
	{
		tmp.writeBefore("(*");
		tmp.write(")");
	}
	res &= applyCast(stmt, writer, tmp);
	if(semicol) writer.write(";");
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
		if(tmp.empty()) continue;
		writer.append(tmp);
		if(i < stmt->getStmts().size() - 1) writer.newLine();
	}
	if(!stmt->isTop()) {
		writer.remIndent();
		writer.newLine();
		writer.write("}");
	}
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
	switch(stmt->getLexValue().getTokVal()) {
	case lex::IDEN: break;
	case lex::TRUE:	 // fallthrough
	case lex::FALSE: // fallthrough
	case lex::NIL:	 // fallthrough
	case lex::INT:	 // fallthrough
	case lex::FLT:	 // fallthrough
	case lex::CHAR:	 // fallthrough
	case lex::STR: writer.write(getConstantDataVar(stmt->getLexValue(), stmt->getValueTy()));
	default: return true;
	}
	// No perma data here as all variables lose permadata attribute
	// in type assign pass for StmtVar
	// The following part is only valid for existing variables.
	// the part for variable declaration exists in Var visit
	writer.write(getMangledName(stmt->getLexValue().getDataStr(), stmt));
	return true;
}
bool CDriver::visit(StmtFnCallInfo *stmt, Writer &writer, const bool &semicol)
{
	return true;
}
bool CDriver::visit(StmtExpr *stmt, Writer &writer, const bool &semicol)
{
	writer.clear();
	if(stmt->getValue()->hasPermaData()) {
		std::string cval;
		if(!getCValue(cval, stmt, stmt->getValue(), stmt->getValueTy())) {
			err.set(stmt, "failed to get C value for scribe value: %s",
				stmt->getValue()->toStr().c_str());
			return false;
		}
		if(!cval.empty()) {
			writer.write(cval);
			return true;
		}
	}

	lex::TokType oper = stmt->getOper().getTokVal();
	Stmt *&lhs	  = stmt->getLHS();
	Stmt *&rhs	  = stmt->getRHS();
	Writer l;
	Writer r;
	if(oper == lex::ARROW || oper == lex::DOT || oper == lex::FNCALL || oper == lex::STCALL ||
	   oper == lex::UAND || oper == lex::UMUL)
	{
		if(!visit(lhs, l, false)) {
			err.set(stmt, "failed to generate C code for LHS in expression");
			return false;
		}
	}
	switch(oper) {
	case lex::ARROW:
	case lex::DOT: {
		StmtSimple *rsim = as<StmtSimple>(rhs);
		if(lhs->getDerefCount()) {
			writer.write("(");
			writer.write(std::string(lhs->getDerefCount(), '*'));
		}
		writer.append(l);
		if(lhs->getDerefCount()) {
			writer.write(")");
		}
		writer.write(".");
		writer.write(rsim->getLexValue().getDataStr());
		break;
	}
	case lex::FNCALL: {
		std::string fname	  = as<StmtSimple>(lhs)->getLexValue().getDataStr();
		std::vector<Stmt *> &args = as<StmtFnCallInfo>(rhs)->getArgs();
		writer.write("%s%" PRIu64 "(", fname.c_str(), lhs->getValueTy()->getID());
		if(!writeCallArgs(stmt->getLoc(), args, lhs->getValueTy(), writer)) return false;
		writer.write(")");
		break;
	}
	case lex::STCALL: {
		std::vector<Stmt *> &args = as<StmtFnCallInfo>(rhs)->getArgs();
		writer.write("(struct_%" PRIu64 "){", lhs->getValueTy()->getID());
		if(!writeCallArgs(stmt->getLoc(), args, lhs->getValueTy(), writer)) return false;
		writer.write("}");
		break;
	}
	// address of
	case lex::UAND: {
		writer.write("(&");
		writer.append(l);
		writer.write(")");
		break;
	}
	// dereference
	case lex::UMUL: {
		writer.write("(*");
		writer.append(l);
		writer.write(")");
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
		if(oper == lex::SUBS && lhs->getValueTy()->isPtr()) {
			if(!visit(lhs, l, false)) {
				err.set(stmt, "failed to generate C code for LHS in expression");
				return false;
			}
			if(!visit(rhs, r, false)) {
				err.set(stmt, "failed to generate C code for LHS in expression");
				return false;
			}
			writer.append(l);
			writer.write("[");
			writer.append(r);
			writer.write("]");
			break;
		}
		if(lhs->getValueTy()->isPrimitiveOrPtr() &&
		   (!rhs || rhs->getValueTy()->isPrimitiveOrPtr())) {
			if(!visit(lhs, l, false)) {
				err.set(stmt, "failed to generate C code for LHS in expression");
				return false;
			}
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
			if(!visit(rhs, r, false)) {
				err.set(stmt, "failed to generate C code for LHS in expression");
				return false;
			}
			writer.append(l);
			writer.write(" %s ", lex::TokStrs[oper]);
			writer.append(r);
			break;
		}
		std::vector<Stmt *> args = {lhs};
		if(rhs) args.push_back(rhs);
		writer.write(getMangledName(optok.getOperCStr(), stmt->getCalledFn()));
		writer.write("(");
		if(!writeCallArgs(stmt->getLoc(), args, stmt->getCalledFn(), writer)) {
			return false;
		}
		writer.write(")");
		break;
	}
	default: err.set(stmt->getOper(), "nonexistent operator"); return false;
	}
	return true;
}
bool CDriver::visit(StmtVar *stmt, Writer &writer, const bool &semicol)
{
	writer.clear();
	std::string varname = stmt->getName().getDataStr();
	if(!stmt->isCodeGenMangled()) varname = getMangledName(varname, stmt);

	if(stmt->getVVal() && stmt->getVVal()->isExtern()) {
		StmtExtern *ext = as<StmtExtern>(stmt->getVVal());
		Stmt *ent	= ext->getEntity();
		if(!ent) {
			std::string macro = "#define " + varname;
			macro += " " + ext->getName().getDataStr();
			macros.push_back(macro);
		} else if(ent->isStructDef()) {
			std::string decl = "typedef " + ext->getName().getDataStr();
			decl += " struct_" + std::to_string(ent->getValueTy()->getID());
			decl += ";";
			typedefs.push_back(decl);
		} else if(ent->isFnSig()) {
			size_t args	  = as<StmtFnSig>(ent)->getArgs().size();
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
			macro += ext->getName().getDataStr() + "(" + argstr + ")";
			macros.push_back(macro);
		}
		if(!visit(stmt->getVVal(), writer, false)) {
			err.set(stmt, "failed to generate C code for extern variable");
			return false;
		}
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->isFnDef()) {
		Writer tmp(writer);
		if(!visit(stmt->getVVal(), tmp, false)) {
			err.set(stmt, "failed to generate C code for function def");
			return false;
		}

		StmtFnDef *fn	 = as<StmtFnDef>(stmt->getVVal());
		StmtType *sigret = fn->getSigRetType();
		std::string retcty;
		if(!getCTypeName(retcty, sigret, sigret->getValueTy(), false, false)) {
			err.set(stmt, "failed to determine C type for scribe type: %s",
				sigret->getValueTy()->toStr().c_str());
			return false;
		}

		tmp.insertAfter(retcty.size(), " " + varname);
		writer.append(tmp);
		// no semicolon after fndef

		// add declaration (at the top) for the function
		Writer decl;
		if(!visit(as<StmtFnDef>(stmt->getVVal())->getSig(), decl, true)) {
			err.set(stmt, "failed to generate C code for function def");
			return false;
		}
		decl.insertAfter(retcty.size(), " " + varname);
		decl.write(";");
		funcdecls.push_back(decl.getData());
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->isStructDef()) {
		// structs are not defined by themselves
		// they are defined when a struct type is encountered
		return true;
	}
	if(stmt->getVVal() && stmt->getValue()->hasData()) {
		// variable is an existing function or struct (FuncVal || TypeTy)
		if(stmt->getValue()->isFunc() || stmt->getValue()->isType()) {
			Writer tmp(writer);
			Stmt *val = stmt->getVVal();
			if(!visit(val, tmp, false)) {
				err.set(stmt, "failed to get C value for scribe value: %s",
					stmt->getValue()->toStr().c_str());
				return false;
			}
			macros.push_back("#define " + varname + " " + tmp.getData());
			return true;
		}
		std::string cty, arrcount, cval;
		Type *t	 = stmt->getValueTy();
		arrcount = getArrCount(t);
		if(!getCTypeName(cty, stmt, t, false, false)) {
			err.set(stmt, "failed to determine C type for scribe type: %s",
				t->toStr().c_str());
			return false;
		}
		if(!getCValue(cval, stmt, stmt->getValue(), stmt->getValueTy())) {
			err.set(stmt, "failed to get C value for scribe value: %s",
				stmt->getValue()->toStr().c_str());
			return false;
		}
		writer.write("%s %s%s = %s", cty.c_str(), varname.c_str(), arrcount.c_str(),
			     cval.c_str());
		return true;
	}

	Writer tmp(writer);
	if(stmt->getVVal() && !visit(stmt->getVVal(), tmp, false)) {
		err.set(stmt, "failed to generate C code from scribe declaration value");
		return false;
	}
	std::string cty, arrcount;
	Type *valty = stmt->getCast() ? stmt->getCast() : stmt->getValueTy();
	arrcount    = getArrCount(valty);
	if(!getCTypeName(cty, stmt, valty, false, false)) {
		err.set(stmt, "unable to determine C type for scribe type: %s",
			valty->toStr().c_str());
		return false;
	}
	writer.write("%s %s%s", cty.c_str(), varname.c_str(), arrcount.c_str());
	if(!tmp.empty()) {
		writer.write(" = ");
		if(stmt->getValueTy()->hasRef()) {
			writer.write("&(");
			writer.append(tmp);
			writer.write(")");
		} else {
			writer.append(tmp);
		}
	}
	return true;
}
bool CDriver::visit(StmtFnSig *stmt, Writer &writer, const bool &semicol)
{
	std::string cty;
	if(!getCTypeName(cty, stmt->getRetType(), stmt->getRetType()->getValueTy(), false, false)) {
		err.set(stmt, "unable to determine C type for scribe type: %s",
			stmt->getValueTy()->toStr().c_str());
		return false;
	}
	writer.write(cty);
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
	// nothing to do of entity
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
		if(!visit(d, tmp, semicol)) {
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
		return true;
	}
	Writer tmp(writer);
	if(!visit(stmt->getVal(), tmp, false)) {
		err.set(stmt, "failed to generate C code for return value");
		return false;
	}
	writer.write("return ");
	if(stmt->getValueTy()->hasRef()) {
		writer.write("&(");
		writer.append(tmp);
		writer.write(")");
	} else {
		writer.append(tmp);
	}
	return true;
}
bool CDriver::visit(StmtContinue *stmt, Writer &writer, const bool &semicol)
{
	writer.write("continue");
	return true;
}
bool CDriver::visit(StmtBreak *stmt, Writer &writer, const bool &semicol)
{
	writer.write("break");
	return true;
}
bool CDriver::visit(StmtDefer *stmt, Writer &writer, const bool &semicol)
{
	err.set(stmt, "defer should never come as a part of code generation");
	return false;
}

const std::string &CDriver::getConstantDataVar(const lex::Lexeme &val, Type *ty)
{
	std::string key;
	std::string value;
	std::string type;
	std::string bits;
	std::string is_sign;
	switch(val.getTokVal()) {
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
		value	= std::to_string(val.getDataInt());
		bits	= std::to_string(as<IntTy>(ty)->getBits());
		is_sign = as<IntTy>(ty)->isSigned() ? "i" : "u";
		key	= value + is_sign + bits;
		type	= "const " + is_sign + bits;
		break;
	case lex::FLT:
		value = std::to_string(val.getDataFlt());
		bits  = std::to_string(as<FltTy>(ty)->getBits());
		key   = value + "f" + bits;
		type  = "const f" + bits;
		break;
	case lex::CHAR:
		value = '\'' + getRawString(val.getDataStr()) + '\'';
		key   = value;
		type  = "const i8";
		break;
	case lex::STR:
		value = '"' + getRawString(val.getDataStr()) + '"';
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
	case VARDECL: return true;
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

bool CDriver::getCTypeName(std::string &res, Stmt *stmt, Type *ty, bool for_decl, bool is_weak)
{
	std::string pre;
	std::string post;
	if(ty->hasStatic()) pre += "static ";
	if(ty->hasConst()) pre += "const ";
	if(ty->hasVolatile()) pre += "volatile ";
	if(ty->hasRef()) post += " *";

	if(is_weak) {
		if(for_decl) res = pre + "struct ";
		res += "struct_" + std::to_string(ty->getID()) + post;
		return true;
	}

	if(ty->isVoid()) {
		res = pre + "void" + post;
		return true;
	}
	if(ty->isTypeTy()) {
		Type *ctyp = as<TypeTy>(ty)->getContainedTy();
		std::string cty;
		if(!getCTypeName(cty, stmt, ctyp, for_decl, is_weak)) {
			err.set(stmt, "failed to determine C type for scribe type: %s",
				ctyp->toStr().c_str());
			return false;
		}
		res = pre + cty + post;
		return true;
	}
	if(ty->isInt()) {
		bool is_signed	 = as<IntTy>(ty)->isSigned();
		std::string bits = std::to_string(as<IntTy>(ty)->getBits());
		res		 = pre + (is_signed ? "i" : "u") + bits + post;
		return true;
	}
	if(ty->isFlt()) {
		std::string bits = std::to_string(as<FltTy>(ty)->getBits());
		res		 = pre + "f" + bits + post;
		return true;
	}
	if(ty->isPtr()) {
		Type *to = as<PtrTy>(ty)->getTo();
		std::string cty;
		if(!getCTypeName(cty, stmt, to, for_decl, as<PtrTy>(ty)->isWeak())) {
			err.set(stmt, "failed to determine C type for scribe type: %s",
				to->toStr().c_str());
			return false;
		}
		res = pre + cty + "*" + post;
		return true;
	}
	if(ty->isFunc()) {
		return getFuncPointer(res, as<FuncTy>(ty), stmt, for_decl, is_weak);
	}
	if(ty->isStruct()) {
		StructTy *s = as<StructTy>(ty);
		if(!addStructDef(stmt, s)) {
			err.set(stmt, "failed to add struct def '%s' in C code",
				s->toStr().c_str());
			return false;
		}
		res = pre + "struct_" + std::to_string(s->getID()) + post;
		return true;
	}
	err.set(stmt, "invalid scribe type encountered: %s", ty->toStr().c_str());
	return false;
}
bool CDriver::getCValue(std::string &res, Stmt *stmt, Value *value, Type *type)
{
	switch(value->getValType()) {
	// in case of VVOID, since res will be empty, it will cause actual expression to emit
	case VVOID: return true;
	case VINT: {
		IntTy *t = as<IntTy>(type);
		if(t->getBits() == 8 && t->isSigned()) {
			res = "'" + std::string(1, as<IntVal>(value)->getVal()) + "'";
			return true;
		}
		res = std::to_string(as<IntVal>(value)->getVal());
		return true;
	}
	case VFLT: res = std::to_string(as<FltVal>(value)->getVal()); return true;
	case VVEC: {
		bool is_str = false;
		assert(type->isPtr());
		if(as<PtrTy>(type)->getTo()->isInt()) {
			IntTy *t = as<IntTy>(as<PtrTy>(type)->getTo());
			if(t->getBits() == 8 && t->isSigned()) is_str = true;
		}
		if(is_str) {
			res = "\"" + as<VecVal>(value)->getAsString() + "\"";
			return true;
		}
		res	 = "{";
		Type *to = as<PtrTy>(type)->getTo();
		for(auto &e : as<VecVal>(value)->getVal()) {
			std::string cval;
			if(!getCValue(cval, stmt, e, to)) {
				err.set(stmt, "failed to determine C value of scribe value: %s",
					e->toStr().c_str());
				return false;
			}
			res += cval + ", ";
		}
		if(as<VecVal>(value)->getVal().size() > 0) {
			res.pop_back();
			res.pop_back();
		}
		res += "}";
		return true;
	}
	case VSTRUCT: {
		StructTy *st = as<StructTy>(type);

		res = "{";
		for(size_t i = 0; i < st->getFields().size(); ++i) {
			Value *fv = as<StructVal>(value)->getField(st->getFieldName(i));
			std::string cval;
			if(!getCValue(cval, stmt, fv, st->getField(i))) {
				err.set(stmt, "failed to determine C value of scribe value: %s",
					fv->toStr().c_str());
				return false;
			}
			res += cval + ", ";
		}
		if(st->getFields().size() > 0) {
			res.pop_back();
			res.pop_back();
		}
		res += "}";
		return true;
	}
	default: {
		err.set(stmt, "failed to generate C value for value: %s", value->toStr().c_str());
		break;
	}
	}
	return false;
}
bool CDriver::addStructDef(Stmt *stmt, StructTy *sty)
{
	static std::unordered_set<uint64_t> declaredstructs;
	if(declaredstructs.find(sty->getID()) != declaredstructs.end()) return true;
	if(sty->isExtern()) { // externed structs are declared in StmtVar
		declaredstructs.insert(sty->getID());
		return true;
	}
	Writer st;
	st.write("struct struct_%" PRIu64 " {", sty->getID());
	if(!sty->getFields().empty()) {
		st.addIndent();
		st.newLine();
	}
	for(size_t i = 0; i < sty->getFields().size(); ++i) {
		std::string cty, arrcount;
		Type *t	 = sty->getField(i);
		arrcount = getArrCount(t);
		if(!getCTypeName(cty, stmt, t, true, false)) {
			err.set(stmt, "failed to determine C type for scribe type: %s",
				t->toStr().c_str());
			return false;
		}
		st.write(cty);
		st.write(" %s%s;", sty->getFieldName(i).c_str(), arrcount.c_str());
		if(i != sty->getFields().size() - 1) st.newLine();
	}
	if(!sty->getFields().empty()) {
		st.remIndent();
		st.newLine();
	}
	st.write("};");
	structdecls.push_back(st.getData());
	Writer tydef;
	tydef.write("typedef struct struct_%" PRIu64 " struct_%" PRIu64 ";", sty->getID(),
		    sty->getID());
	structdecls.push_back(tydef.getData());
	declaredstructs.insert(sty->getID());
	return true;
}
bool CDriver::applyCast(Stmt *stmt, Writer &writer, Writer &tmp)
{
	if(stmt->getCast()) {
		writer.write("(");
		std::string cty;
		// bool has_ref = stmt->getCast()->hasRef();
		// stmt->getCast()->unsetRef();
		if(!getCTypeName(cty, stmt, stmt->getCast(), false, false)) {
			err.set(stmt, "received invalid c type name for scribe cast type: %s",
				stmt->getCast()->toStr().c_str());
			return false;
		}
		// if(has_ref) stmt->getCast()->setRef();
		writer.write(cty);
		writer.write(")(");
		writer.append(tmp);
		writer.write(")");
	} else {
		writer.append(tmp);
	}
	return true;
}
bool CDriver::writeCallArgs(const ModuleLoc &loc, const std::vector<Stmt *> &args, Type *ty,
			    Writer &writer)
{
	FuncTy *fn   = nullptr;
	StructTy *st = nullptr;
	if(ty->isFunc()) fn = as<FuncTy>(ty);
	if(ty->isStruct()) st = as<StructTy>(ty);
	for(size_t i = 0; i < args.size(); ++i) {
		Stmt *a	 = args[i];
		Type *at = fn ? fn->getArg(i) : st->getField(i);
		Writer tmp(writer);
		Type *cast = a->getCast();
		a->castTo(nullptr);
		if(!visit(a, tmp, false)) {
			err.set(loc, "failed to generate C code for func call argument");
			return false;
		}
		a->castTo(cast);
		if(a->getDerefCount()) {
			tmp.writeBefore("(" + std::string(a->getDerefCount(), '*'));
			tmp.write(")");
		}
		if(at->hasRef()) {
			tmp.writeBefore("&(");
			tmp.write(")");
		}
		if(!applyCast(a, writer, tmp)) return false;
		if(i != args.size() - 1) writer.write(", ");
	}
	return true;
}
bool CDriver::getFuncPointer(std::string &res, FuncTy *f, Stmt *stmt, bool for_decl, bool is_weak)
{
	static std::unordered_set<uint64_t> funcids;
	std::string decl = "typedef ";
	std::string cty;
	res = "func_" + std::to_string(f->getID());
	if(funcids.find(f->getID()) != funcids.end()) return true;
	if(!getCTypeName(cty, stmt, f->getRet(), for_decl, is_weak)) {
		err.set(stmt, "failed to determine C type for scribe type: %s",
			f->getRet()->toStr().c_str());
		return false;
	}
	decl += cty;
	decl += " (*" + res + ")";
	decl += "(";
	for(auto &t : f->getArgs()) {
		cty.clear();
		if(!getCTypeName(cty, stmt, t, for_decl, is_weak)) {
			err.set(stmt, "failed to determine C type for scribe type: %s",
				t->toStr().c_str());
			return false;
		}
		decl += cty;
		decl += ", ";
	}
	if(f->getArgs().size() > 0) {
		decl.pop_back();
		decl.pop_back();
	}
	decl += ");";
	typedefs.push_back(decl);
	funcids.insert(f->getID());
	return true;
}
std::string CDriver::getArrCount(Type *&t)
{
	std::string res;
	while(t->isPtr() && as<PtrTy>(t)->getCount()) {
		res = res + "[" + std::to_string(as<PtrTy>(t)->getCount()) + "]";
		t   = as<PtrTy>(t)->getTo();
	}
	return res;
}
std::string CDriver::getSystemCompiler()
{
	std::string compiler = env::get("C_COMPILER");
	if(!compiler.empty()) {
		if(compiler.front() == '/' || compiler.front() == '~' || compiler.front() == '.') {
			compiler = fs::absPath(compiler);
			return compiler;
		}
	} else {
		compiler = "clang";
	}
	compiler = env::getExeFromPath(compiler);
	if(compiler.empty()) {
		compiler = "gcc";
		compiler = env::getExeFromPath(compiler);
	}
	return compiler;
}
} // namespace sc