/*
	MIT License
	Copyright (c) 2022 Scribe Language Repositories
	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include <limits.h>

#include "Config.hpp"
#include "FS.hpp"
#include "Intrinsics.hpp"
#include "Parser.hpp"
#include "Passes/TypeAssign.hpp"
#include "ValueMgr.hpp"

#define GetType(i) args[i]->getType()

#define GetIntVal(i) as<IntVal>(args[i]->getValue())->getVal()
#define CreateIntVal(v) IntVal::create(c, v)
#define GetFltVal(i) as<FltVal>(args[i]->getValue())->getVal()
#define CreateFltVal(v) FltVal::create(c, v)

namespace sc
{
static bool IsValidSource(std::string &modname);
static size_t SizeOf(Type *ty);

INTRINSIC(import)
{
	if(!args[0]->getValue()->hasData() || !args[0]->getValue()->isStrLiteral()) {
		err.set(stmt, "import must be a compile time computable string");
		return false;
	}
	std::string modname = as<VecVal>(args[0]->getValue())->getAsString();
	if(modname.empty()) {
		err.set(stmt, "invalid comptime value for module string");
		return false;
	}

	if(!IsValidSource(modname)) {
		err.set(stmt, "Error: import file '%s' does not exist", modname.c_str());
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
	stmt->createAndSetValue(NamespaceVal::create(c, mod->getID()));
	return true;
}
INTRINSIC(ismainsrc)
{
	bool ismm = stmt->getMod()->isMainModule();
	stmt->createAndSetValue(IntVal::create(c, mkI1Ty(c), CDTRUE, ismm));
	return true;
}
INTRINSIC(isprimitive)
{
	bool is_prim = args[0]->getValueTy()->isPrimitive();
	stmt->createAndSetValue(IntVal::create(c, mkI1Ty(c), CDPERMA, is_prim));
	return true;
}
INTRINSIC(isprimitiveorptr)
{
	bool is_prim_or_ptr = args[0]->getValueTy()->isPrimitiveOrPtr();
	stmt->createAndSetValue(IntVal::create(c, mkI1Ty(c), CDPERMA, is_prim_or_ptr));
	return true;
}
INTRINSIC(iscstring)
{
	bool is_cstr = args[0]->getValue()->isStrLiteral();
	stmt->createAndSetValue(IntVal::create(c, mkI1Ty(c), CDPERMA, is_cstr));
	return true;
}
INTRINSIC(iscchar)
{
	bool is_cchar = args[0]->getValueTy()->isInt();
	if(is_cchar) {
		IntTy *t = as<IntTy>(args[0]->getValueTy());
		is_cchar &= t->getBits() == 8;
		is_cchar &= t->isSigned();
	}
	stmt->createAndSetValue(IntVal::create(c, mkI1Ty(c), CDPERMA, is_cchar));
	return true;
}
INTRINSIC(isequalty)
{
	bool is_same_ty = args[0]->getValueTy()->getID() == args[1]->getValueTy()->getID();
	stmt->createAndSetValue(IntVal::create(c, mkI1Ty(c), CDPERMA, is_same_ty));
	return true;
}
INTRINSIC(szof)
{
	int64_t sz = SizeOf(args[0]->getValueTy());
	if(!sz) {
		err.set(args[0], "invalid type info, received size 0");
		return false;
	}
	stmt->createAndSetValue(IntVal::create(c, mkU64Ty(c), CDPERMA, sz));
	return true;
}
INTRINSIC(as)
{
	// args[0] must have a value of type TypeVal
	args[1]->castTo(as<TypeVal>(args[0]->getValue())->getVal());
	*source = args[1];
	return true;
}
INTRINSIC(typeof)
{
	stmt->createAndSetValue(TypeVal::create(c, args[0]->getValueTy()));
	return true;
}
INTRINSIC(ptr)
{
	// args[0] should be a TypeVal
	Type *res = as<TypeVal>(args[0]->getValue())->getVal()->clone(c);
	res	  = PtrTy::create(c, res, 0, false);
	stmt->createAndSetValue(TypeVal::create(c, res));
	return true;
}
INTRINSIC(ref)
{
	// args[0] should be a TypeVal
	Type *res = as<TypeVal>(args[0]->getValue())->getVal()->clone(c);
	res->setRef();
	stmt->createAndSetValue(TypeVal::create(c, res));
	return true;
}
INTRINSIC(cons)
{
	// args[0] should be a TypeVal
	Type *res = as<TypeVal>(args[0]->getValue())->getVal()->clone(c);
	res->setConst();
	stmt->createAndSetValue(TypeVal::create(c, res));
	return true;
}
INTRINSIC(valen)
{
	if(stmt->getValue()->hasData()) return true;
	TypeAssignPass *ta = c.getPass<TypeAssignPass>();
	if(!ta->isFnVALen()) {
		err.set(stmt, "this is not a variadic function");
		return false;
	}
	size_t vasz = ta->getFnVALen();
	stmt->createAndSetValue(IntVal::create(c, mkU64Ty(c), CDPERMA, vasz));
	return true;
}
INTRINSIC(array)
{
	std::vector<int64_t> counts;
	Type *resty = as<TypeVal>(args[0]->getValue())->getVal();

	for(size_t i = 1; i < args.size(); ++i) {
		counts.insert(counts.begin(), as<IntVal>(args[i]->getValue())->getVal());
	}
	for(auto &count : counts) {
		resty = PtrTy::create(c, resty, count, false);
	}
	Value *res = resty->toDefaultValue(c, err, stmt->getLoc(), CDPERMA);
	if(!res) {
		err.set(stmt, "failed to get default value from array's type");
		return false;
	}
	stmt->createAndSetValue(res);
	return true;
}
INTRINSIC(assn_ptr)
{
	args[0]->updateValue(c, args[1]->getValue());
	stmt->updateValue(c, args[0]->getValue());
	return true;
}
// enum is:
//   Unknown = 0
//   Linux = 1
//   Windows = 2
//   Apple = 3
//   Android = 4
//   FreeBSD = 5
//   NetBSD = 6
//   OpenBSD = 7
//   DragonFly = 8
INTRINSIC(getosid)
{
	int res = 0;
#if defined(__linux__)
	res = 1;
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	res = 2;
#elif defined(__APPLE__)
	res = 3;
#elif defined(__ANDROID__)
	res = 4;
#elif defined(__FreeBSD__)
	res = 5;
#elif defined(__NetBSD__)
	res = 6;
#elif defined(__OpenBSD__)
	res = 7;
#elif defined(__DragonFly__)
	res = 8;
#endif
	stmt->createAndSetValue(IntVal::create(c, mkI32Ty(c), CDPERMA, res));
	return true;
}
INTRINSIC(syspathmax)
{
	stmt->createAndSetValue(IntVal::create(c, mkI32Ty(c), CDPERMA, PATH_MAX));
	return true;
}
INTRINSIC(compileerror)
{
	std::string e;
	for(auto &a : args) {
		if(a->getValue()->hasData() && a->getValue()->isStrLiteral()) {
			e += as<VecVal>(a->getValue())->getAsString();
		} else {
			e += a->getValue()->toStr().c_str();
		}
	}
	err.set(stmt, e.c_str());
	return false;
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

static size_t SizeOf(Type *ty)
{
	if(ty->isPtr()) {
		return sizeof(void *);
	}
	if(ty->isInt()) {
		return as<IntTy>(ty)->getBits() / 8;
	}
	if(ty->isFlt()) {
		return as<FltTy>(ty)->getBits() / 8;
	}
	if(ty->isStruct()) {
		StructTy *st   = as<StructTy>(ty);
		size_t sz      = 0;
		size_t biggest = 0;
		for(auto &t : st->getFields()) {
			size_t newsz = SizeOf(t);
			if(newsz > biggest) biggest = newsz;
			sz += newsz;
		}
		while(sz % biggest != 0) {
			++sz;
		}
		return sz;
	}
	return 0;
}
} // namespace sc