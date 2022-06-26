/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "CodeGen/C.hpp"

#include <cstddef>
#include <inttypes.h>

#include "CodeGen/C/Prelude.hpp"
#include "Env.hpp"
#include "FS.hpp"
#include "Parser.hpp"
#include "Utils.hpp"

namespace sc
{
CTy::CTy()
	: recurse(0), ptrs(0), ptrsin(0), isstatic(false), isvolatile(false), isconst(false),
	  isref(false), iscast(false), isdecl(false), isweak(false)
{}
CTy::CTy(const String &base, const String &arr, size_t ptrs)
	: base(base), arr(arr), recurse(0), ptrs(ptrs), ptrsin(0), isstatic(false),
	  isvolatile(false), isconst(false), isref(false), iscast(false), isdecl(false),
	  isweak(false)
{}

CTy CTy::operator+(const CTy &other) const
{
	return CTy(base + other.base, arr + other.arr, ptrs + other.ptrs);
}
CTy &CTy::operator+=(const CTy &other)
{
	base += other.base;
	arr += other.arr;
	ptrs += other.ptrs;
	return *this;
}

String CTy::toStr(StringRef *varname)
{
	static String res;
	res.clear();
	if(isstatic) res += "static ";
	if(isvolatile) res += "volatile ";
	if(isconst) res += "const ";
	res += base;
	res += " ";
	if(ptrs - ptrsin) {
		res.append(ptrs - ptrsin, '*');
	}
	if(!arr.empty() && (ptrsin || isref)) {
		res += "(";
	}
	res.append(ptrsin, '*');
	if(isref) res += "*";
	if(varname) res += *varname;
	if(!arr.empty() && (ptrsin || isref)) {
		res += ")";
	}
	res += arr;
	return res;
}

size_t CTy::size()
{
	size_t sz = 0;
	if(isstatic) sz += 7;	// "static "
	if(isvolatile) sz += 9; // "volatile "
	if(isconst) sz += 6;	// "const "
	if(isref) ++sz;		// "*"
	sz += base.size() + ptrs + arr.size();
	// space after base (see toStr())
	++sz;
	return sz;
}

void CTy::clear()
{
	base.clear();
	arr.clear();
	ptrs	   = 0;
	isstatic   = false;
	isvolatile = false;
	isconst	   = false;
	isref	   = false;
}

CDriver::CDriver(RAIIParser &parser)
	: CodeGenDriver(parser), preheadermacros(default_preheadermacros),
	  headers(default_includes), typedefs(default_typedefs)
{}
CDriver::~CDriver() {}

bool CDriver::compile(StringRef outfile)
{
	Module *mainmod = parser.getModule(parser.getModuleStack().front());
	Writer mainwriter;
	if(!visit(mainmod->getParseTree(), mainwriter, false)) {
		err::out(mainmod->getParseTree(),
			 {"failed to compile module: ", mainmod->getPath()});
		return false;
	}
	Writer finalmod;
	for(auto &ph : preheadermacros) {
		finalmod.write(ph);
		finalmod.newLine();
	}
	if(preheadermacros.size() > 0) finalmod.newLine();
	for(auto &h : headers) {
		finalmod.write({"#include ", h});
		finalmod.newLine();
	}
	if(headers.size() > 0) finalmod.newLine();
	for(auto &m : macros) {
		finalmod.write(m);
		finalmod.newLine();
	}
	if(macros.size() > 0) finalmod.newLine();
	finalmod.write(default_macro_magic);
	finalmod.newLine();
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
	for(auto &d : funcptrs) {
		finalmod.write(d);
		finalmod.newLine();
	}
	if(funcptrs.size() > 0) finalmod.newLine();
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
	StringRef opt		 = "0";
	StringRef std		 = "11";
	bool ir_only		 = cliargs.has("ir");
	bool llir		 = cliargs.has("llir");
	if(cliargs.has("opt")) {
		StringRef res = cliargs.val("opt");
		if(res.empty()) {
			err::out(mainmod->getParseTree(),
				 {"optimization option must have a value, found nothing"});
			return false;
		}
		if(res != "0" && res != "1" && res != "2" && res != "3" && res != "s" && res != "z")
		{
			err::out(mainmod->getParseTree(),
				 {"optimization option value must be one of"
				  " 0, 1, 2, 3, s, or z; found: ",
				  res});
			return false;
		}
		opt = res;
	}
	if(cliargs.has("std")) {
		StringRef res = cliargs.val("std");
		if(res.empty()) {
			err::out(mainmod->getParseTree(),
				 {"standard option must have a value, found nothing"});
			return false;
		}
		if(res != "11" && res != "14" && res != "17") {
			err::out(mainmod->getParseTree(), {"standard option value must be"
							   " one of 11, 14, or 17; found nothing"});
			return false;
		}
		std = res;
	}

	String tmpfile;
	if(ir_only) {
		tmpfile = outfile;
		tmpfile += ".c";
	} else {
		auto loc = outfile.find_last_of('/');
		tmpfile	 = (loc == String::npos ? outfile : outfile.substr(loc));
		tmpfile	 = "/tmp/" + tmpfile + ".c";
	}
	FILE *f = fopen(tmpfile.c_str(), "w+");
	if(!f) {
		err::out(mainmod->getParseTree(),
			 {"failed to create file for writing C code: ", tmpfile});
		return false;
	}
	fprintf(f, "%s\n", finalmod.getData().c_str());
	fclose(f);
	if(ir_only) return true;

	StringRef compiler = getSystemCompiler();
	String cmd;
	cmd.reserve(128);
	cmd += compiler;
	cmd += " -std=c";
	cmd += std;
	cmd += " -O";
	cmd += opt;
	cmd += " ";
	if(opt == "0") cmd += "-g ";
	for(auto &h : headerflags) {
		cmd += h;
		cmd += " ";
	}
	for(auto &l : libflags) {
		cmd += l;
		cmd += " ";
	}
	cmd += tmpfile + " -o ";
	cmd += outfile;
	if(llir) cmd += ".ll -S -emit-llvm";
	int res = std::system(cmd.c_str());
	res	= WEXITSTATUS(res);
	if(res) {
		StringRef resref = ctx.strFrom((int64_t)res);
		err::out(mainmod->getParseTree(),
			 {"failed to compile code, got compiler exit status: ", resref});
		return false;
	}
	return true;
}

bool CDriver::visit(Stmt *stmt, Writer &writer, bool semicol)
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
	case FOR: res = visit(as<StmtFor>(stmt), tmp, semicol); break;
	case RET: res = visit(as<StmtRet>(stmt), tmp, semicol); break;
	case CONTINUE: res = visit(as<StmtContinue>(stmt), tmp, semicol); break;
	case BREAK: res = visit(as<StmtBreak>(stmt), tmp, semicol); break;
	case DEFER: res = visit(as<StmtDefer>(stmt), tmp, semicol); break;
	default: {
		err::out(stmt, {"invalid statement found for C code generation: %s",
				stmt->getStmtTypeCString()});
		break;
	}
	}
	if(!res) return false;
	if(tmp.empty()) return true;
	if(!semicol &&
	   (stmt->isExpr() ||
	    (stmt->isSimple() && as<StmtSimple>(stmt)->getLexValue().getTokVal() == lex::IDEN)) &&
	   stmt->isRef())
	{
		tmp.writeBefore("(*");
		tmp.write(")");
	}
	res &= applyCast(stmt, writer, tmp);
	if(semicol) writer.write(";");
	return res;
}

bool CDriver::visit(StmtBlock *stmt, Writer &writer, bool semicol)
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
			err::out(stmt, {"failed to generate IR for block"});
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
bool CDriver::visit(StmtType *stmt, Writer &writer, bool semicol)
{
	writer.clear();
	return false;
}
bool CDriver::visit(StmtSimple *stmt, Writer &writer, bool semicol)
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
	case lex::STR: writer.write(getConstantDataVar(stmt->getLexValue(), stmt->getTy(true)));
	default: return true;
	}
	// No perma data here as all variables lose permadata attribute
	// in type assign pass for StmtVar
	// The following part is only valid for existing variables.
	// the part for variable declaration exists in Var visit
	writer.write(getMangledName(stmt->getLexValue().getDataStr(), stmt));
	return true;
}
bool CDriver::visit(StmtFnCallInfo *stmt, Writer &writer, bool semicol)
{
	return true;
}
bool CDriver::visit(StmtExpr *stmt, Writer &writer, bool semicol)
{
	writer.clear();
	if(stmt->getVal() && stmt->getVal()->hasPermaData()) {
		String cval;
		if(!getCValue(cval, stmt, stmt->getVal(), stmt->getTy())) {
			err::out(stmt, {"failed to get C value for scribe value: ",
					stmt->getVal()->toStr()});
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
			err::out(stmt, {"failed to generate C code for LHS in expression"});
			return false;
		}
	}
	switch(oper) {
	case lex::ARROW:
	case lex::DOT: {
		StmtSimple *rsim = as<StmtSimple>(rhs);
		if(lhs->getDerefCount()) {
			writer.write("(");
			writer.write(lhs->getDerefCount(), '*');
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
		StringRef fname	     = as<StmtSimple>(lhs)->getLexValue().getDataStr();
		Vector<Stmt *> &args = as<StmtFnCallInfo>(rhs)->getArgs();
		writer.write(fname);
		writer.write(lhs->getTy()->getUniqID());
		writer.write("(");
		if(!writeCallArgs(stmt->getLoc(), args, lhs->getTy(), writer)) return false;
		writer.write(")");
		break;
	}
	case lex::STCALL: {
		Vector<Stmt *> &args = as<StmtFnCallInfo>(rhs)->getArgs();
		writer.write("(struct_");
		writer.write(lhs->getTy()->getUniqID());
		writer.write("){");
		if(!writeCallArgs(stmt->getLoc(), args, lhs->getTy(), writer)) return false;
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
		if(oper == lex::SUBS && lhs->getTy()->isPtr()) {
			if(!visit(lhs, l, false)) {
				err::out(stmt, {"failed to generate C code for LHS in expression"});
				return false;
			}
			if(!visit(rhs, r, false)) {
				err::out(stmt, {"failed to generate C code for RHS in expression"});
				return false;
			}
			writer.append(l);
			writer.write("[");
			writer.append(r);
			writer.write("]");
			break;
		}
		if(lhs->getTy()->isPrimitiveOrPtr() && (!rhs || rhs->getTy()->isPrimitiveOrPtr())) {
			if(!visit(lhs, l, false)) {
				err::out(stmt, {"failed to generate C code for LHS in expression"});
				return false;
			}
			if(optok.isUnaryPre()) {
				writer.write(optok.getUnaryNoCharCStr());
				writer.write("(");
				writer.append(l);
				writer.write(")");
				break;
			}
			if(optok.isUnaryPost()) {
				writer.write("(");
				writer.append(l);
				writer.write(")");
				writer.write(optok.getUnaryNoCharCStr());
				break;
			}
			if(!visit(rhs, r, false)) {
				err::out(stmt, {"failed to generate C code for RHS in expression"});
				return false;
			}
			if(oper != lex::ASSN) writer.write("(");
			writer.append(l);
			if(oper != lex::ASSN) writer.write(")");
			writer.write({" ", lex::TokStrs[oper], " "});
			if(oper != lex::ASSN) writer.write("(");
			writer.append(r);
			if(oper != lex::ASSN) writer.write(")");
			break;
		}
		Vector<Stmt *> args = {lhs};
		if(rhs) args.push_back(rhs);
		writer.write(getMangledName(optok.getOperCStr(), stmt->getCalledFn()));
		writer.write("(");
		if(!writeCallArgs(stmt->getLoc(), args, stmt->getCalledFn(), writer)) {
			return false;
		}
		writer.write(")");
		break;
	}
	default: err::out(stmt->getOper(), {"nonexistent operator"}); return false;
	}
	return true;
}
bool CDriver::visit(StmtVar *stmt, Writer &writer, bool semicol)
{
	writer.clear();
	StringRef varname = stmt->getName().getDataStr();
	if(!stmt->isCodeGenMangled()) varname = getMangledName(varname, stmt);

	if(stmt->getVVal() && stmt->getVVal()->isExtern()) {
		StmtExtern *ext	  = as<StmtExtern>(stmt->getVVal());
		Stmt *ent	  = ext->getEntity();
		StringRef extname = ext->getName().getDataStr();
		if(!ent) {
			macros.push_back(ctx.strFrom({"#define ", varname, " ", extname}));
		} else if(ent->isStructDef()) {
			String uniqid = std::to_string(ent->getTy()->getUniqID());
			StringRef res = ctx.strFrom({"typedef ", extname, " struct_", uniqid, ";"});
			typedefs.push_back(res);
		} else if(ent->isFnSig()) {
			size_t args  = as<StmtFnSig>(ent)->getArgs().size();
			String macro = "#define ";
			macro += varname;
			macro += "(";
			String argstr;
			for(size_t i = 0; i < args; ++i) {
				argstr += 'a' + i;
				argstr += ", ";
			}
			if(args) {
				argstr.pop_back();
				argstr.pop_back();
			}
			macro += argstr;
			macro += ") ";
			macro += extname;
			macro += "(";
			macro += argstr;
			macro += ")";
			macros.push_back(ctx.moveStr(std::move(macro)));
		}
		if(!visit(stmt->getVVal(), writer, false)) {
			err::out(stmt, {"failed to generate C code for extern variable"});
			return false;
		}
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->isFnDef()) {
		Writer tmp(writer);
		if(!visit(stmt->getVVal(), tmp, false)) {
			err::out(stmt, {"failed to generate C code for function def"});
			return false;
		}

		StmtFnDef *fn	 = as<StmtFnDef>(stmt->getVVal());
		StmtType *sigret = fn->getSigRetType();
		CTy retcty;
		if(!getCType(retcty, sigret, sigret->getTy())) {
			err::out(stmt, {"failed to determine C type for scribe type: ",
					sigret->getTy()->toStr()});
			return false;
		}
		retcty.setConst(sigret->isConst());
		retcty.setRef(sigret->isRef());

		tmp.insertAfter(retcty.size(), varname);
		tmp.insertAfter(retcty.size() + varname.size(), " ");
		if(fn->isInline()) tmp.writeBefore("_SC_INLINE_ ");
		writer.append(tmp);
		// no semicolon after fndef

		// add declaration (at the top) for the function
		Writer decl;
		if(!visit(as<StmtFnDef>(stmt->getVVal())->getSig(), decl, true)) {
			err::out(stmt, {"failed to generate C code for function def"});
			return false;
		}
		decl.insertAfter(retcty.size(), varname);
		decl.insertAfter(retcty.size() + varname.size(), " ");
		if(fn->isInline()) decl.writeBefore("_SC_INLINE_ ");
		decl.write(";");
		funcdecls.push_back(ctx.moveStr(std::move(decl.getData())));
		return true;
	}
	if(stmt->getVVal() && stmt->getVVal()->isStructDef()) {
		// structs are not defined by themselves
		// they are defined when a struct type is encountered
		return true;
	}
	if(stmt->getVVal() && stmt->getVal() && stmt->getVal()->hasData()) {
		// variable is an existing function (FuncVal)
		if(stmt->getVal()->isFunc()) {
			Writer tmp(writer);
			Stmt *val = stmt->getVVal();
			if(!visit(val, tmp, false)) {
				err::out(stmt, {"failed to get C value for scribe value: ",
						stmt->getVal()->toStr()});
				return false;
			}
			macros.push_back(ctx.strFrom({"#define ", varname, " ", tmp.getData()}));
			return true;
		}
		// variable is an existing type/struct (TypeVal)
		// this is not required as type aliases are internal to the language
		// and are not propagated over to the C code
		if(stmt->getVal()->isType()) {
			// Writer tmp(writer);
			// Type *t = stmt->getTy();
			// CTy cty;
			// if(!getCType(cty, stmt, t)) {
			// 	err::out(stmt, {"failed to determine C "
			// 			"type for scribe type: ",
			// 			t->toStr()});
			// 	return false;
			// }
			// typedefs.push_back(
			// ctx.strFrom({"typedef ", cty.toStr(nullptr), " ", varname, ";"}));
			return true;
		}
		CTy cty;
		String cval;
		Type *t = stmt->getTy();
		if(!getCType(cty, stmt, t)) {
			err::out(stmt,
				 {"failed to determine C type for scribe type: ", t->toStr()});
			return false;
		}
		if(!getCValue(cval, stmt, stmt->getVal(), stmt->getTy())) {
			err::out(stmt, {"failed to get C value for scribe value: ",
					stmt->getVal()->toStr()});
			return false;
		}
		cty.setStatic(stmt->isStatic());
		cty.setVolatile(stmt->isVolatile());
		cty.setConst(stmt->isConst());
		cty.setRef(stmt->isRef());
		writer.write({cty.toStr(&varname), " = ", cval});
		return true;
	}

	Writer tmp(writer);
	if(stmt->getVVal() && !visit(stmt->getVVal(), tmp, false)) {
		err::out(stmt, {"failed to generate C code from scribe declaration value"});
		return false;
	}
	// check if value is itself an externed variable, if so, make this a macro
	if(stmt->getVVal() && stmt->getVVal()->isSimple()) {
		StmtSimple *sim = as<StmtSimple>(stmt->getVVal());
		if(sim->getDecl() && sim->getDecl()->getVVal() &&
		   sim->getDecl()->getVVal()->isExtern()) {
			macros.push_back(ctx.strFrom({"#define ", varname, " ", tmp.getData()}));
			return true;
		}
	}
	CTy cty;
	Type *valty = stmt->getCast() ? stmt->getCast() : stmt->getTy();
	if(!getCType(cty, stmt, valty)) {
		err::out(stmt, {"unable to determine C type for scribe type: ", valty->toStr()});
		return false;
	}
	cty.setStatic(stmt->isStatic());
	cty.setVolatile(stmt->isVolatile());
	cty.setConst(stmt->isConst());
	cty.setRef(stmt->isRef());
	writer.write(cty.toStr(&varname));
	if(!tmp.empty()) {
		writer.write(" = ");
		if(stmt->isRef()) {
			writer.write("&(");
			writer.append(tmp);
			writer.write(")");
		} else {
			writer.append(tmp);
		}
	}
	return true;
}
bool CDriver::visit(StmtFnSig *stmt, Writer &writer, bool semicol)
{
	CTy cty;
	Stmt *retty = stmt->getRetType();
	if(!getCType(cty, retty, retty->getTy())) {
		err::out(stmt,
			 {"unable to determine C type for scribe type: ", stmt->getTy()->toStr()});
		return false;
	}
	cty.setConst(retty->isConst());
	cty.setRef(retty->isRef());
	writer.write(cty.toStr(nullptr));
	writer.write("(");
	for(size_t i = 0; i < stmt->getArgs().size(); ++i) {
		auto &a = stmt->getArgs()[i];
		Writer tmp(writer);
		if(!visit(a, tmp, false)) {
			err::out(stmt, {"failed to generate C code for function arg"});
			return false;
		}
		writer.append(tmp);
		if(i < stmt->getArgs().size() - 1) writer.write(", ");
	}
	writer.write(")");
	return true;
}
bool CDriver::visit(StmtFnDef *stmt, Writer &writer, bool semicol)
{
	if(!visit(stmt->getSig(), writer, false)) {
		err::out(stmt, {"failed to generate C code for function"});
		return false;
	}
	if(!stmt->getBlk()) return true;
	writer.write(" ");
	if(!visit(stmt->getBlk(), writer, false)) {
		err::out(stmt, {"failed to generate C code for function block"});
		return false;
	}
	return true;
}
bool CDriver::visit(StmtHeader *stmt, Writer &writer, bool semicol)
{
	if(!stmt->getNames().getDataStr().empty()) {
		Vector<StringRef> headersf = stringDelim(stmt->getNames().getDataStr(), ",");
		for(auto &h : headersf) {
			bool has = false;
			for(auto &hf : headers) {
				if(h == hf) {
					has = true;
					break;
				}
			}
			if(!has) headers.push_back(h);
		}
	}
	if(!stmt->getFlags().getDataStr().empty()) {
		headerflags.push_back(stmt->getFlags().getDataStr());
	}
	return true;
}
bool CDriver::visit(StmtLib *stmt, Writer &writer, bool semicol)
{
	if(!stmt->getFlags().getDataStr().empty()) {
		libflags.push_back(stmt->getFlags().getDataStr());
	}
	return true;
}
bool CDriver::visit(StmtExtern *stmt, Writer &writer, bool semicol)
{
	if(stmt->getHeaders()) visit(stmt->getHeaders(), writer, false);
	if(stmt->getLibs()) visit(stmt->getLibs(), writer, false);
	// nothing to do of entity
	return true;
}
bool CDriver::visit(StmtEnum *stmt, Writer &writer, bool semicol)
{
	err::out(stmt, {"Unimplemented enum C code generation"});
	return false;
}
bool CDriver::visit(StmtStruct *stmt, Writer &writer, bool semicol)
{
	// structs are not defined by themselves
	// they are defined when a struct type is encountered
	return true;
}
bool CDriver::visit(StmtVarDecl *stmt, Writer &writer, bool semicol)
{
	for(size_t i = 0; i < stmt->getDecls().size(); ++i) {
		auto &d = stmt->getDecls()[i];
		Writer tmp(writer);
		if(!visit(d, tmp, semicol)) {
			err::out(stmt, {"failed to generate C code for var decl"});
			return false;
		}
		writer.append(tmp);
		if(i < stmt->getDecls().size() - 1) {
			writer.write(";");
			writer.newLine();
		}
	}
	return true;
}
bool CDriver::visit(StmtCond *stmt, Writer &writer, bool semicol)
{
	if(stmt->isInline()) {
		if(stmt->getConditionals().empty()) return true;
		if(!visit(stmt->getConditionals().back().getBlk(), writer, false)) {
			err::out(stmt, {"failed to generate C code for inline conditional block"});
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
				err::out(c.getCond(),
					 {"failed to generate C code for conditional"});
				return false;
			}
			writer.append(tmp);
			writer.write(") ");
		}
		if(!visit(c.getBlk(), writer, false)) {
			err::out(stmt, {"failed to generate C code for conditional block"});
			return false;
		}
	}
	return true;
}
bool CDriver::visit(StmtFor *stmt, Writer &writer, bool semicol)
{
	if(stmt->isInline()) {
		if(stmt->getBlk()->getStmts().empty()) return true;
		Writer tmp(writer);
		if(!visit(stmt->getBlk(), tmp, false)) {
			err::out(stmt, {"failed to generate C code for inline for-loop block"});
			return false;
		}
		writer.append(tmp);
		return true;
	}
	writer.write("for(");
	if(stmt->getInit()) {
		Writer tmp(writer);
		if(!visit(stmt->getInit(), tmp, false)) {
			err::out(stmt, {"failed to generate C code for for-loop init"});
			return false;
		}
		writer.append(tmp);
	}
	writer.write(";");
	if(stmt->getCond()) {
		writer.write(" ");
		Writer tmp(writer);
		if(!visit(stmt->getCond(), tmp, false)) {
			err::out(stmt, {"failed to generate C code for for-loop condition"});
			return false;
		}
		writer.append(tmp);
	}
	writer.write(";");
	if(stmt->getIncr()) {
		writer.write(" ");
		Writer tmp(writer);
		if(!visit(stmt->getIncr(), tmp, false)) {
			err::out(stmt, {"failed to generate C code for for-loop incr"});
			return false;
		}
		writer.append(tmp);
	}
	writer.write(") ");
	Writer tmp(writer);
	if(!visit(stmt->getBlk(), tmp, false)) {
		err::out(stmt, {"failed to generate C code for for-loop block"});
		return false;
	}
	writer.append(tmp);
	return true;
}
bool CDriver::visit(StmtRet *stmt, Writer &writer, bool semicol)
{
	if(!stmt->getRetVal()) {
		writer.write("return");
		return true;
	}
	Writer tmp(writer);
	if(!visit(stmt->getRetVal(), tmp, false)) {
		err::out(stmt, {"failed to generate C code for return value"});
		return false;
	}
	writer.write("return ");
	if(stmt->isRef()) {
		writer.write("&(");
		writer.append(tmp);
		writer.write(")");
	} else {
		writer.append(tmp);
	}
	return true;
}
bool CDriver::visit(StmtContinue *stmt, Writer &writer, bool semicol)
{
	writer.write("continue");
	return true;
}
bool CDriver::visit(StmtBreak *stmt, Writer &writer, bool semicol)
{
	writer.write("break");
	return true;
}
bool CDriver::visit(StmtDefer *stmt, Writer &writer, bool semicol)
{
	err::out(stmt, {"defer should never come as a part of code generation"});
	return false;
}

StringRef CDriver::getConstantDataVar(const lex::Lexeme &val, Type *ty)
{
	String key;
	String value;
	String type;
	String bits;
	String is_sign;
	switch(val.getTokVal()) {
	case lex::TRUE:
		value = "1";
		key += value;
		key += "i1";
		type = "const i1";
		break;
	case lex::FALSE: // fallthrough
	case lex::NIL:
		value = "0";
		key += value;
		key += "i1";
		type = "const i1";
		break;
	case lex::INT:
		value	= std::to_string(val.getDataInt());
		bits	= std::to_string(as<IntTy>(ty)->getBits());
		is_sign = as<IntTy>(ty)->isSigned() ? "i" : "u";
		key += value;
		key += as<IntTy>(ty)->isSigned() ? "i" : "u";
		key += bits;
		type += "const ";
		type += as<IntTy>(ty)->isSigned() ? "i" : "u";
		type += bits;
		break;
	case lex::FLT:
		value = std::to_string(val.getDataFlt());
		bits  = std::to_string(as<FltTy>(ty)->getBits());
		key += value;
		key += "f";
		key += bits;
		type += "const f";
		type += bits;
		break;
	case lex::CHAR:
		value += '\'';
		appendRawString(value, val.getDataStr());
		value += '\'';
		key  = value;
		type = "const i8";
		break;
	case lex::STR:
		value += '"';
		appendRawString(value, val.getDataStr());
		value += '"';
		key  = value;
		type = "const i8*";
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
	StringRef k    = ctx.moveStr(std::move(key));
	StringRef var  = getNewConstantVar();
	StringRef decl = ctx.strFrom({type, " ", var, " = ", value, ";"});
	constants[k]   = {var, decl};
	return constants[k].var;
}
StringRef CDriver::getNewConstantVar()
{
	static size_t const_id = 0;
	return ctx.strFrom({"const_", std::to_string(const_id++)});
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
	case FOR: return false;
	case RET: return true;
	case CONTINUE: return true;
	case BREAK: return true;
	case DEFER: return true;
	}
	return false;
}

bool CDriver::getCType(CTy &cty, Stmt *stmt, Type *ty)
{
	if(cty.isTop()) {
		size_t ptrsin = 0;
		cty.arr	      = getArrCount(ty, ptrsin);
		cty.setPtrsIn(ptrsin);
	}
	cty.incRecurse();

	if(cty.isWeak()) {
		if(cty.isDecl()) cty.base = "struct ";
		cty.base += "struct_";
		cty.base += std::to_string(ty->getUniqID());
		return true;
	}

	if(ty->isVoid()) {
		cty.base = "void";
		return true;
	}
	if(ty->isTypeTy()) {
		Type *ctyp = as<TypeTy>(ty)->getContainedTy();
		if(!getCType(cty, stmt, ctyp)) {
			err::out(stmt,
				 {"failed to determine C type for scribe type: ", ctyp->toStr()});
			return false;
		}
		return true;
	}
	if(ty->isInt()) {
		cty.base = (as<IntTy>(ty)->isSigned() ? "i" : "u");
		cty.base += std::to_string(as<IntTy>(ty)->getBits());
		return true;
	}
	if(ty->isFlt()) {
		cty.base = "f";
		cty.base += std::to_string(as<FltTy>(ty)->getBits());
		return true;
	}
	if(ty->isPtr()) {
		Type *to = as<PtrTy>(ty)->getTo();
		cty.setWeak(as<PtrTy>(ty)->isWeak());
		if(!getCType(cty, stmt, to)) {
			err::out(stmt,
				 {"failed to determine C type for scribe type: ", to->toStr()});
			return false;
		}
		if(!as<PtrTy>(ty)->getCount()) cty.incPtrs();
		return true;
	}
	if(ty->isFunc()) {
		return getFuncPointer(cty, as<FuncTy>(ty), stmt);
	}
	if(ty->isStruct()) {
		StructTy *s = as<StructTy>(ty);
		if(!addStructDef(stmt, s)) {
			err::out(stmt, {"failed to add struct def '", s->toStr(), "' in C code"});
			return false;
		}
		cty.base = "struct_";
		cty.base += std::to_string(s->getUniqID());
		return true;
	}
	err::out(stmt, {"invalid scribe type encountered: ", ty->toStr()});
	return false;
}
bool CDriver::getCValue(String &res, Stmt *stmt, Value *value, Type *type, bool i8_to_char)
{
	switch(value->getValType()) {
	case VINT: {
		IntTy *t = as<IntTy>(type);
		if(i8_to_char && t->getBits() == 8 && t->isSigned() &&
		   as<IntVal>(value)->getVal() > 31) {
			if(as<IntVal>(value)->getVal() == '\'' ||
			   as<IntVal>(value)->getVal() == '\\') {
				res = "'\\";
				res.append(1, as<IntVal>(value)->getVal());
				res += "'";
			} else {
				res = "'";
				res.append(1, as<IntVal>(value)->getVal());
				res += "'";
			}
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
			if(as<PtrTy>(type)->isArrayPtr()) is_str = false;
		}
		if(is_str) {
			res = "\"";
			res += as<VecVal>(value)->getAsString();
			res += "\"";
			return true;
		}
		res	 = "{";
		Type *to = as<PtrTy>(type)->getTo();
		for(auto &e : as<VecVal>(value)->getVal()) {
			String cval;
			if(!getCValue(cval, stmt, e, to, is_str)) {
				err::out(stmt, {"failed to determine C value of scribe value: ",
						e->toStr()});
				return false;
			}
			res += cval;
			res += ", ";
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
			String cval;
			if(!getCValue(cval, stmt, fv, st->getField(i))) {
				err::out(stmt, {"failed to determine C value of scribe value: ",
						fv->toStr()});
				return false;
			}
			res += cval;
			res += ", ";
		}
		if(st->getFields().size() > 0) {
			res.pop_back();
			res.pop_back();
		}
		res += "}";
		return true;
	}
	default: {
		err::out(stmt, {"failed to generate C value for value: ", value->toStr()});
		break;
	}
	}
	return false;
}
bool CDriver::addStructDef(Stmt *stmt, StructTy *sty)
{
	static Set<uint64_t> declaredstructs;
	if(declaredstructs.find(sty->getUniqID()) != declaredstructs.end()) return true;
	if(sty->isExtern()) { // externed structs are declared in StmtVar
		declaredstructs.insert(sty->getUniqID());
		return true;
	}
	StmtStruct *stdecl = sty->getDecl();
	Writer st;
	String uniqid = std::to_string(sty->getUniqID());
	st.write({"struct struct_", uniqid, " {"});
	if(!sty->getFields().empty()) {
		st.addIndent();
		st.newLine();
	}
	for(size_t i = 0; i < sty->getFields().size(); ++i) {
		CTy cty;
		StringRef fieldname = sty->getFieldName(i);
		Type *t		    = sty->getField(i);
		cty.setDecl(true);
		if(!getCType(cty, stmt, t)) {
			err::out(stmt,
				 {"failed to determine C type for scribe type: ", t->toStr()});
			return false;
		}
		cty.setStatic(stdecl && stdecl->getField(i)->isStatic());
		cty.setVolatile(stdecl && stdecl->getField(i)->isVolatile());
		cty.setConst(stdecl && stdecl->getField(i)->isConst());
		st.write({cty.toStr(&fieldname), ";"});
		if(i != sty->getFields().size() - 1) st.newLine();
	}
	if(!sty->getFields().empty()) {
		st.remIndent();
		st.newLine();
	}
	st.write("};");
	structdecls.push_back(ctx.moveStr(std::move(st.getData())));
	Writer tydef;
	String uid = std::to_string(sty->getUniqID());
	tydef.write({"typedef struct struct_", uid, " struct_", uid, ";"});
	structdecls.push_back(ctx.moveStr(std::move(tydef.getData())));
	declaredstructs.insert(sty->getUniqID());
	return true;
}
bool CDriver::applyCast(Stmt *stmt, Writer &writer, Writer &tmp)
{
	if(stmt->getCast()) {
		writer.write("(");
		CTy cty;
		cty.setCast(true);
		if(!getCType(cty, stmt, stmt->getCast())) {
			err::out(stmt, {"received invalid c type name for scribe cast type: ",
					stmt->getCast()->toStr()});
			return false;
		}
		cty.setConst(stmt->isCastConst());
		cty.setRef(stmt->isCastRef());
		// don't write something like (i8 [30])
		if(cty.isArray() && !cty.getPtrs() && !cty.isRef()) {
			cty.clearArray();
			cty.incPtrs();
		}
		writer.write(cty.toStr(nullptr));
		writer.write(")(");
		writer.append(tmp);
		writer.write(")");
	} else {
		writer.append(tmp);
	}
	return true;
}
bool CDriver::writeCallArgs(const ModuleLoc *loc, const Vector<Stmt *> &args, Type *ty,
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
		Type *cast	 = a->getCast();
		uint8_t castmask = a->getCastStmtMask();
		a->castTo(nullptr, (uint8_t)0);
		if(!visit(a, tmp, false)) {
			err::out(loc, {"failed to generate C code for func call argument"});
			return false;
		}
		a->castTo(cast, castmask);
		if(a->getDerefCount()) {
			// equivalent to ( *... )
			// writeBefore must be used in reverse order of actual sequence
			tmp.writeBefore(a->getDerefCount(), '*');
			tmp.writeBefore("(");
			tmp.write(")");
		}
		if(fn && fn->getSig() && fn->getSig()->getArg(i)->isRef()) {
			tmp.writeBefore("&(");
			tmp.write(")");
		}
		if(!applyCast(a, writer, tmp)) return false;
		if(i != args.size() - 1) writer.write(", ");
	}
	return true;
}
bool CDriver::getFuncPointer(CTy &res, FuncTy *f, Stmt *stmt)
{
	static Set<uint64_t> funcids;
	res.base = "func_";
	res.base += std::to_string(f->getUniqID());
	if(funcids.find(f->getUniqID()) != funcids.end()) return true;

	String decl = "typedef ";
	CTy cty;
	cty.setCast(res.isCast());
	cty.setDecl(res.isDecl());
	cty.setWeak(res.isWeak());
	if(!getCType(cty, stmt, f->getRet())) {
		err::out(stmt,
			 {"failed to determine C type for scribe type: ", f->getRet()->toStr()});
		return false;
	}
	cty.setConst(f->getSig() && f->getSig()->getRetType()->isConst());
	cty.setRef(f->getSig() && f->getSig()->getRetType()->isRef());
	decl += cty.toStr(nullptr);
	decl += " (*";
	decl += res.base;
	decl += ")(";
	for(size_t i = 0; i < f->getArgs().size(); ++i) {
		Type *t	     = f->getArg(i);
		StmtVar *arg = f->getSig() ? f->getSig()->getArg(i) : nullptr;
		cty.clear();
		if(!getCType(cty, stmt, t)) {
			err::out(stmt,
				 {"failed to determine C type for scribe type: ", t->toStr()});
			return false;
		}
		cty.setConst(arg && arg->isConst());
		cty.setRef(arg && arg->isRef());
		decl += cty.toStr(nullptr);
		decl += ", ";
	}
	if(f->getArgs().size() > 0) {
		decl.pop_back();
		decl.pop_back();
	}
	decl += ");";
	funcptrs.push_back(ctx.moveStr(std::move(decl)));
	funcids.insert(f->getUniqID());
	return true;
}
StringRef CDriver::getArrCount(Type *t, size_t &ptrsin)
{
	String res;
	bool enable_ptrsin = true;
	while(t->isPtr()) {
		if(!as<PtrTy>(t)->getCount()) {
			t = as<PtrTy>(t)->getTo();
			if(enable_ptrsin) ++ptrsin;
			continue;
		}
		enable_ptrsin = false;
		res += "[";
		res += std::to_string(as<PtrTy>(t)->getCount());
		res += "]";
		t = as<PtrTy>(t)->getTo();
	}
	return ctx.moveStr(std::move(res));
}
StringRef CDriver::getSystemCompiler()
{
	String compiler = env::get("C_COMPILER");
	if(!compiler.empty()) {
		if(compiler.front() == '/' || compiler.front() == '~' || compiler.front() == '.') {
			compiler = fs::absPath(compiler);
			return ctx.moveStr(std::move(compiler));
		}
	} else {
		compiler = "clang";
	}
	compiler = env::getExeFromPath(compiler);
	if(compiler.empty()) {
		compiler = "gcc";
		compiler = env::getExeFromPath(compiler);
	}
	return ctx.moveStr(std::move(compiler));
}
StringRef CDriver::getMangledName(StringRef name, Type *ty)
{
	String res = std::to_string(ty->getUniqID());
	res.insert(res.begin(), name.begin(), name.end());
	return ctx.moveStr(std::move(res));
}
} // namespace sc