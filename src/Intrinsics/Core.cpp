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

#include "Config.hpp"
#include "FS.hpp"
#include "Intrinsics.hpp"
#include "Parser.hpp"
#include "Passes/ValueAssign.hpp"
#include "TypeMgr.hpp"

#define GetType(i) args[i]->getType()

#define GetIntVal(i) as<IntVal>(args[i]->getValue())->getVal()
#define CreateIntVal(v) IntVal::create(c, v)
#define GetFltVal(i) as<FltVal>(args[i]->getValue())->getVal()
#define CreateFltVal(v) FltVal::create(c, v)

namespace sc
{
static bool IsValidSource(std::string &modname);

INTRINSIC(import)
{
	if(!args[0]->getValue() || args[0]->getValue()->getType() != VVEC) {
		err.set(stmt, "import must be a compile time computable string");
		return false;
	}
	std::string modname = as<VecVal>(args[0]->getValue())->getAsString();
	if(modname.empty()) {
		err.set(stmt, "invalid comptime value for module string");
		return false;
	}

	if(!IsValidSource(modname)) {
		err.set(stmt, "Error: import file %s does not exist", modname.c_str());
		return false;
	}

	RAIIParser *parser = c.getParser();
	Module *mod	   = nullptr;
	Module *topmod	   = nullptr;
	StmtBlock *blk	   = nullptr;
	StmtBlock *topblk  = nullptr;
	if(parser->hasModule(modname)) {
		mod = parser->getModule(modname);
		goto gen_import;
	}
	if(!parser->parse(modname)) {
		err.set(stmt, "failed to parse source: %s", modname.c_str());
		return false;
	}
	mod    = parser->getModule(modname);
	topmod = parser->getModule(*parser->getModuleStack().begin());
	blk    = as<StmtBlock>(mod->getParseTree());
	topblk = as<StmtBlock>(topmod->getParseTree());

gen_import:
	stmt->setType(ImportTy::create(c, mod->getID()));
	return true;
}
INTRINSIC(ismainsrc)
{
	stmt->setVal(IntVal::create(c, stmt->getMod()->isMainModule()));
	return true;
}
INTRINSIC(isprimitive)
{
	stmt->setVal(IntVal::create(c, args[0]->getType()->isPrimitive()));
	return true;
}
INTRINSIC(as)
{
	stmt->setType(args[1]->getType());
	stmt->castTo(args[0]->getType());
	if(args[1]->getValue()) stmt->setVal(args[1]->getValue());
	return true;
}
INTRINSIC(valen)
{
	if(!args[0]->getType()->isVariadic()) {
		err.set(stmt, "expected variadic type for valen(), found: %s",
			args[0]->getType()->toStr().c_str());
		return false;
	}
	stmt->setVal(IntVal::create(c, as<VariadicTy>(args[0]->getType())->getArgs().size()));
	return true;
}
INTRINSIC(array)
{
	std::vector<int64_t> counts;
	Type *argty  = args[0]->getType();
	Type *stmtty = stmt->getType();

	if(currintrin == IPARSE) goto stage_parse;
	if(currintrin == IVALUE) goto stage_value;

stage_parse:
	for(size_t i = 1; i < args.size(); ++i) {
		counts.insert(counts.begin(), as<IntVal>(args[i]->getValue())->getVal());
	}
	for(auto &count : counts) {
		argty = PtrTy::create(c, argty, count);
	}
	stmt->setType(argty);
	return true;

stage_value:
	Value *res = stmtty->toDefaultValue(c, err, stmt->getLoc());
	if(!res) {
		err.set(stmt, "failed to get default value from array's type");
		return false;
	}
	stmt->setVal(res);
	return true;
}

static bool IsValidSource(std::string &modname)
{
	static std::string import_dir = INSTALL_DIR "/include/scribe";
	if(modname.front() != '~' && modname.front() != '/' && modname.front() != '.') {
		if(fs::exists(import_dir + "/" + modname + ".sc")) {
			modname = fs::absPath(import_dir + "/" + modname + ".sc");
			return true;
		}
	} else {
		if(modname.front() == '~') {
			modname.erase(modname.begin());
			std::string home = fs::home();
			modname.insert(modname.begin(), home.begin(), home.end());
		}
		if(fs::exists(modname + ".sc")) {
			modname = fs::absPath(modname + ".sc");
			return true;
		}
	}
	return false;
}
} // namespace sc