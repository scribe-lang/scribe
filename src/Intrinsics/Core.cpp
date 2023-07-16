#include "Config.hpp"
#include "Env.hpp"
#include "FS.hpp"
#include "Intrinsics.hpp"
#include "Parser.hpp"
#include "Passes/TypeAssign.hpp"

#define GetType(i) args[i]->getType()

#define GetIntVal(i) as<IntVal>(args[i]->getVal())->getVal()
#define CreateIntVal(v) IntVal::create(c, v)
#define GetFltVal(i) as<FltVal>(args[i]->getVal())->getVal()
#define CreateFltVal(v) FltVal::create(c, v)

namespace sc
{
static StringRef GetCompilerID(Context &c);
static bool IsValidSource(String &modname);
static size_t SizeOf(Type *ty);

INTRINSIC(compilerid)
{
	static StringRef id = GetCompilerID(c);
	VecVal *res	    = VecVal::createStr(c, CDPERMA, id);
	stmt->setVal(res);
	stmt->setConst();
	return true;
}
INTRINSIC(compilerpath)
{
	static String path = env::getProcPath();
	VecVal *res	   = VecVal::createStr(c, CDPERMA, path);
	stmt->setVal(res);
	stmt->setConst();
	return true;
}

INTRINSIC(import)
{
	if(!args[0]->getTy()->isStrLiteral() || !args[0]->getVal() || !args[0]->getVal()->isVec()) {
		err::out(stmt, {"import must be a compile time computable string"});
		return false;
	}
	String modname = as<VecVal>(args[0]->getVal())->getAsString();
	if(modname.empty()) {
		err::out(stmt, {"invalid comptime value for module string"});
		return false;
	}

	if(!IsValidSource(modname)) {
		err::out(stmt, {"Error: import file '", modname, "' does not exist"});
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
		err::out(stmt, {"failed to parse source: ", modname});
		return false;
	}
	mod    = parser->getModule(modname);
	topmod = parser->getModule(*parser->getModuleStack().begin());
	blk    = as<StmtBlock>(mod->getParseTree());
	topblk = as<StmtBlock>(topmod->getParseTree());

gen_import:
	stmt->setTyVal(PtrTy::getStr(c), NamespaceVal::create(c, mod->getID()));
	return true;
}
INTRINSIC(ismainsrc)
{
	bool ismm = stmt->getMod()->isMainModule();
	stmt->setVal(IntVal::create(c, CDPERMA, ismm));
	return true;
}
INTRINSIC(isprimitive)
{
	bool is_prim = args[0]->getTy()->isPrimitive();
	stmt->setVal(IntVal::create(c, CDPERMA, is_prim));
	return true;
}
INTRINSIC(isptr)
{
	bool is_ptr = args[0]->getTy()->isPtr();
	stmt->setVal(IntVal::create(c, CDPERMA, is_ptr));
	return true;
}
INTRINSIC(isprimitiveorptr)
{
	bool is_prim_or_ptr = args[0]->getTy()->isPrimitiveOrPtr();
	stmt->setVal(IntVal::create(c, CDPERMA, is_prim_or_ptr));
	return true;
}
INTRINSIC(isint)
{
	bool is_int = args[0]->getTy()->isInt();
	stmt->setVal(IntVal::create(c, CDPERMA, is_int));
	return true;
}
INTRINSIC(isintsigned)
{
	bool is_signed = args[0]->getTy()->isInt();
	if(is_signed) is_signed = as<IntTy>(args[0]->getTy())->isSigned();
	stmt->setVal(IntVal::create(c, CDPERMA, is_signed));
	return true;
}
INTRINSIC(isflt)
{
	bool is_flt = args[0]->getTy()->isFlt();
	stmt->setVal(IntVal::create(c, CDPERMA, is_flt));
	return true;
}
INTRINSIC(iscchar)
{
	bool is_cchar = args[0]->getTy()->isInt();
	if(is_cchar) {
		IntTy *t = as<IntTy>(args[0]->getTy());
		is_cchar &= t->getBits() == 8;
		is_cchar &= t->isSigned();
	}
	stmt->setVal(IntVal::create(c, CDPERMA, is_cchar));
	return true;
}
INTRINSIC(iscstring)
{
	bool is_cstr = args[0]->getTy()->isStrLiteral();
	stmt->setVal(IntVal::create(c, CDPERMA, is_cstr));
	return true;
}
INTRINSIC(isequalty)
{
	bool is_same_ty = args[0]->getTy()->getID() == args[1]->getTy()->getID();
	stmt->setVal(IntVal::create(c, CDPERMA, is_same_ty));
	return true;
}
INTRINSIC(szof)
{
	int64_t sz = SizeOf(args[0]->getTy());
	if(!sz) {
		err::out(args[0], {"invalid type info, received size 0"});
		return false;
	}
	stmt->setVal(IntVal::create(c, CDPERMA, sz));
	return true;
}
INTRINSIC(as)
{
	// args[0] must have a value of type TypeVal
	if(!args[0]->getVal() || !args[0]->getVal()->isType()) {
		err::out(args[0], {"expected a type for the first argument to @as()"});
		return false;
	}
	args[1]->castTo(as<TypeVal>(args[0]->getVal())->getVal(), args[0]->getStmtMask());
	*source = args[1];
	return true;
}
INTRINSIC(typeof)
{
	stmt->setTypeVal(c, args[0]->getTy());
	return true;
}
INTRINSIC(ptr)
{
	// args[0] should be a TypeVal
	if(!args[0]->getVal() || !args[0]->getVal()->isType()) {
		err::out(args[0], {"expected a type for the first argument to @ptr()"});
		return false;
	}
	Type *res = as<TypeVal>(args[0]->getVal())->getVal()->specialize(c);
	res	  = PtrTy::get(c, res, 0, false);
	stmt->setTypeVal(c, res);
	return true;
}
INTRINSIC(valen)
{
	TypeAssignPass *ta = c.getPass<TypeAssignPass>();
	if(!ta->isFnVALen()) {
		err::out(stmt, {"this is not a variadic function"});
		return false;
	}
	size_t vasz = ta->getFnVALen();
	stmt->setVal(IntVal::create(c, CDPERMA, vasz));
	return true;
}
INTRINSIC(array)
{
	if(!args[0]->getVal() || !args[0]->getVal()->isType()) {
		err::out(args[0], {"expected array type for the first argument"});
		return false;
	}
	Vector<int64_t> counts;
	Type *resty = as<TypeVal>(args[0]->getVal())->getVal();

	for(size_t i = 1; i < args.size(); ++i) {
		counts.insert(counts.begin(), as<IntVal>(args[i]->getVal())->getVal());
	}
	for(auto &count : counts) {
		resty = PtrTy::get(c, resty, count, false);
		if(!resty) {
			err::out(stmt, {"failed to create array - 0 count provided"});
			return false;
		}
	}
	Value *res = resty->toDefaultValue(c, stmt->getLoc(), CDPERMA);
	if(!res) {
		err::out(stmt, {"failed to get default value from array's type"});
		return false;
	}
	stmt->setTyVal(resty, res);
	return true;
}
INTRINSIC(enumtagty)
{
	if(!args[0]->getVal() || !args[0]->getVal()->isNamespace()) {
		err::out(args[0], {"expected an enum for the first argument"});
		return false;
	}
	Value *v	   = args[0]->getVal();
	TypeAssignPass *ta = c.getPass<TypeAssignPass>();
	Type *ty	   = ta->getEnumTagTy(as<NamespaceVal>(v)->getVal());
	if(!ty) {
		err::out(stmt, {"invalid enum encountered: ", as<NamespaceVal>(v)->getVal()});
		return false;
	}
	stmt->setTypeVal(c, ty);
	return true;
}
INTRINSIC(assn_ptr)
{
	err::out(stmt, {"assn pointer intrinsic does not work on values"});
	return false;
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
#if defined(OS_LINUX)
	res = 1;
#elif defined(OS_WINDOWS)
	res = 2;
#elif defined(OS_APPLE)
	res = 3;
#elif defined(OS_ANDROID)
	res = 4;
#elif defined(OS_FREEBSD)
	res = 5;
#elif defined(OS_NETBSD)
	res = 6;
#elif defined(OS_OPENBSD)
	res = 7;
#elif defined(OS_DRAGONFLYBSD)
	res = 8;
#endif
	stmt->setVal(IntVal::create(c, CDPERMA, res));
	return true;
}
INTRINSIC(syspathmax)
{
	stmt->setVal(IntVal::create(c, CDPERMA, SCRIBE_PATH_MAX));
	return true;
}
INTRINSIC(compileerror)
{
	String e;
	for(auto &a : args) {
		if(a->getTy()->isStrLiteral() && a->getVal() && a->getVal()->isVec()) {
			e += as<VecVal>(a->getVal())->getAsString();
		} else {
			e += a->getVal()->toStr();
		}
	}
	err::out(stmt, {e});
	return false;
}
INTRINSIC(setmaxcompilererr)
{
	size_t maxerr = as<IntVal>(args[0]->getVal())->getVal();
	err::setMaxErrs(maxerr);
	*source = nullptr;
	return true;
}

static StringRef GetCompilerID(Context &c)
{
	StringRef MA = COMPILER_MAJOR_S;
	StringRef MI = COMPILER_MINOR_S;
	StringRef PA = COMPILER_PATCH_S;
	StringRef id =
	c.strFrom({MA, ".", MI, ".", PA, " (", REPO_URL, " ", COMMIT_ID, " [", TREE_STATUS, "])"});
	return id;
}

static bool IsValidSource(String &modname)
{
	static String import_dir = INSTALL_DIR "/include/scribe";
	if(modname.front() != '~' && modname.front() != '/' && modname.front() != '.') {
		if(fs::exists(import_dir + "/" + modname + ".sc")) {
			modname = fs::absPath(import_dir + "/" + modname + ".sc");
			return true;
		}
	} else {
		if(modname.front() == '~') {
			modname.erase(modname.begin());
			String home = fs::home();
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
	static constexpr size_t szvoidp = sizeof(void *);

	if(ty->isPtr()) {
		uint16_t count = as<PtrTy>(ty)->getCount();
		if(count) return count * SizeOf(as<PtrTy>(ty)->getTo());
		return sizeof(void *);
	}
	if(ty->isInt()) {
		if(as<IntTy>(ty)->getBits() < 8) return 1;
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
			// biggest cannot be greater than sizeof(void *)
			if(newsz > biggest && biggest < szvoidp) {
				biggest = newsz > szvoidp ? szvoidp : newsz;
			}
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