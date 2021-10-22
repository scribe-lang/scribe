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

#include "Types.hpp"

#include "Parser/Stmts.hpp"

namespace sc
{
uint64_t genTypeID()
{
	static uint64_t id = _LAST;
	return id++;
}

size_t getPointerCount(Type *t);
Type *applyPointerCount(Context &c, Type *t, const size_t &count);

static const char *TypeStrs[] = {
"void",	      "<any>",	  "int",    "flt",	  "<template>", "<ptr>",     "<array>",
"<function>", "<struct>", "<enum>", "<variadic>", "<import>",	"<funcmap>",
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Base Type
///////////////////////////////////////////////////////////////////////////////////////////////////

Type::Type(const Types &type, const size_t &info, const uint64_t &id)
	: type(type), info(info), id(id)
{}
Type::~Type() {}
bool Type::isBaseCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(isAny()) return true;
	bool is_lhs_prim = isPrimitive();
	bool is_rhs_prim = rhs->isPrimitive();
	size_t lhs_ptr	 = getPointerCount(this);
	size_t rhs_ptr	 = getPointerCount(rhs);
	bool num_to_num	 = false;
	if(!lhs_ptr && !rhs_ptr) {
		num_to_num = is_lhs_prim && is_rhs_prim;
	}
	if(!num_to_num && getID() != rhs->getID()) {
		e.set(loc, "different type ids (LHS: %s, RHS: %s) not compatible", toStr().c_str(),
		      rhs->toStr().c_str());
		return false;
	}
	if(!lhs_ptr && rhs_ptr) {
		e.set(loc, "cannot use a pointer type (RHS: %s) against non pointer (LHS: %s)",
		      rhs->toStr().c_str(), toStr().c_str());
		return false;
	}
	if(!rhs_ptr && rhs_ptr && !is_rhs_prim) {
		e.set(loc,
		      "non pointer type (RHS: %s) cannot be assigned to pointer type (LHS: %s)",
		      rhs->toStr().c_str(), toStr().c_str());
		return false;
	}
	if(rhs_ptr != lhs_ptr && !is_rhs_prim) {
		e.set(loc, "inequal pointer assignment here (LHS: %s, RHS: %s)", toStr().c_str(),
		      rhs->toStr().c_str());
		return false;
	}
	if(rhs->hasConst() && !hasConst()) {
		e.set(loc, "losing constness here, cannot continue (LHS: %s, RHS: %s)",
		      toStr().c_str(), rhs->toStr().c_str());
		return false;
	}
	if(rhs->hasVariadic() && !hasVariadic()) {
		e.set(loc, "cannot assign variadic type to non variadic (LHS: %s, RHS: %s)",
		      toStr().c_str(), rhs->toStr().c_str());
		return false;
	}
	return true;
}
std::string Type::infoToStr()
{
	std::string res;
	if(info & REF) res += "&";
	if(info & STATIC) res += "static ";
	if(info & CONST) res += "const ";
	if(info & VOLATILE) res += "volatile ";
	if(info & VARIADIC) res = "...";
	return res;
}
std::string Type::baseToStr()
{
	return infoToStr() + TypeStrs[type] + "@" + std::to_string(id);
}
std::string Type::toStr()
{
	return baseToStr();
}
bool Type::isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	return isBaseCompatible(rhs, e, loc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Core Types
///////////////////////////////////////////////////////////////////////////////////////////////////

#define BasicTypeDefine(Ty, EnumName)                                  \
	Ty::Ty() : Type(EnumName, 0, EnumName) {}                      \
	Ty::Ty(const size_t &info) : Type(EnumName, info, EnumName) {} \
	Ty::~Ty() {}                                                   \
	Type *Ty::clone(Context &c)                                    \
	{                                                              \
		return c.allocType<Ty>(getInfo());                     \
	}                                                              \
	Ty *Ty::create(Context &c)                                     \
	{                                                              \
		return c.allocType<Ty>();                              \
	}

BasicTypeDefine(VoidTy, TVOID);
BasicTypeDefine(AnyTy, TANY);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Int Type
///////////////////////////////////////////////////////////////////////////////////////////////////

IntTy::IntTy(const size_t &bits, const bool &sign) : Type(TINT, 0, TINT), bits(bits), sign(sign) {}
IntTy::IntTy(const size_t &info, const uint64_t &id, const size_t &bits, const bool &sign)
	: Type(TINT, 0, id), bits(bits), sign(sign)
{}
IntTy::~IntTy() {}

Type *IntTy::clone(Context &c)
{
	return c.allocType<IntTy>(getInfo(), getID(), bits, sign);
}
std::string IntTy::toStr()
{
	return infoToStr() + (sign ? "i" : "u") + std::to_string(bits);
}

IntTy *IntTy::create(Context &c, const size_t &_bits, const bool &_sign)
{
	return c.allocType<IntTy>(_bits, _sign);
}

const size_t &IntTy::getBits() const
{
	return bits;
}

const bool &IntTy::isSigned() const
{
	return sign;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Float Type
///////////////////////////////////////////////////////////////////////////////////////////////////

FltTy::FltTy(const size_t &bits) : Type(TFLT, 0, TFLT), bits(bits) {}
FltTy::FltTy(const size_t &info, const uint64_t &id, const size_t &bits)
	: Type(TFLT, 0, id), bits(bits)
{}
FltTy::~FltTy() {}

Type *FltTy::clone(Context &c)
{
	return c.allocType<FltTy>(getInfo(), getID(), bits);
}
std::string FltTy::toStr()
{
	return infoToStr() + "f" + std::to_string(bits);
}

FltTy *FltTy::create(Context &c, const size_t &_bits)
{
	return c.allocType<FltTy>(_bits);
}

const size_t &FltTy::getBits() const
{
	return bits;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Type Type
///////////////////////////////////////////////////////////////////////////////////////////////////

TypeTy::TypeTy(Type *containedty) : Type(TTYPE, 0, TTYPE), containedty(containedty) {}
TypeTy::TypeTy(const size_t &info, const uint64_t &id, Type *containedty)
	: Type(TTYPE, 0, id), containedty(containedty)
{}
TypeTy::~TypeTy() {}

Type *TypeTy::clone(Context &c)
{
	return c.allocType<TypeTy>(getInfo(), getID(), containedty->clone(c));
}
std::string TypeTy::toStr()
{
	return infoToStr() + "type<" + (containedty ? containedty->toStr() : "(none)") + ">";
}

TypeTy *TypeTy::create(Context &c, Type *_containedty)
{
	return c.allocType<TypeTy>(_containedty);
}

Type *TypeTy::getContainedTy()
{
	return containedty;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Pointer Type
///////////////////////////////////////////////////////////////////////////////////////////////////

PtrTy::PtrTy(Type *to) : Type(TPTR, 0, TPTR), to(to) {}
PtrTy::PtrTy(const size_t &info, const uint64_t &id, Type *to) : Type(TPTR, info, id), to(to) {}
PtrTy::~PtrTy() {}
Type *PtrTy::clone(Context &c)
{
	return c.allocType<PtrTy>(getInfo(), getID(), to->clone(c));
}
std::string PtrTy::toStr()
{
	return "*" + infoToStr();
}
PtrTy *PtrTy::create(Context &c, Type *ptr_to)
{
	return c.allocType<PtrTy>(ptr_to);
}
Type *PtrTy::getTo()
{
	return to;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Array Type
///////////////////////////////////////////////////////////////////////////////////////////////////

ArrayTy::ArrayTy(const uint64_t &count, Type *of) : Type(TARRAY, 0, TARRAY), count(count), of(of) {}
ArrayTy::ArrayTy(const size_t &info, const uint64_t &id, const uint64_t &count, Type *of)
	: Type(TARRAY, info, id), count(count), of(of)
{}
ArrayTy::~ArrayTy() {}
Type *ArrayTy::clone(Context &c)
{
	return c.allocType<ArrayTy>(getInfo(), getID(), count, of->clone(c));
}
std::string ArrayTy::toStr()
{
	return infoToStr() + "[" + std::to_string(count) + "]" + of->toStr();
}
bool ArrayTy::isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(rhs, e, loc)) return false;
	ArrayTy *r = as<ArrayTy>(rhs);
	if(count != r->count) {
		e.set(loc, "incompatible count between arrays (LHS: %s, RHS: %s)", toStr().c_str(),
		      rhs->toStr().c_str());
		return false;
	}
	if(!of->isCompatible(r->getOf(), e, loc)) {
		e.set(loc, "incompatible type between arrays (LHS: %s, RHS: %s)", toStr().c_str(),
		      rhs->toStr().c_str());
		return false;
	}
	return true;
}
ArrayTy *ArrayTy::create(Context &c, const uint64_t &arr_count, Type *arr_of)
{
	return c.allocType<ArrayTy>(arr_count, arr_of);
}
const uint64_t &ArrayTy::getCount()
{
	return count;
}
Type *ArrayTy::getOf()
{
	return of;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Struct Type
///////////////////////////////////////////////////////////////////////////////////////////////////

StructTy::StructTy(const std::vector<std::string> &fieldnames, const std::vector<Type *> &fields)
	: Type(TSTRUCT, 0, genTypeID()), fields(fields), is_def(true)
{
	for(size_t i = 0; i < fieldnames.size(); ++i) {
		fieldpos[fieldnames[i]] = i;
	}
}
StructTy::StructTy(const size_t &info, const uint64_t &id,
		   const std::unordered_map<std::string, size_t> &fieldpos,
		   const std::vector<Type *> &fields, const bool &is_def)
	: Type(TSTRUCT, info, id), fieldpos(fieldpos), fields(fields), is_def(is_def)
{}
StructTy::~StructTy() {}
Type *StructTy::clone(Context &c)
{
	std::vector<Type *> newfields;
	for(auto &field : fields) newfields.push_back(field->clone(c));
	return c.allocType<StructTy>(getInfo(), getID(), fieldpos, newfields, is_def);
}
std::string StructTy::toStr()
{
	std::string res = infoToStr() + "struct<" + std::to_string(getID()) + ">{";
	for(auto &f : fields) {
		res += f->toStr() + ", ";
	}
	if(fields.size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += "}";
	return res;
}
bool StructTy::isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(rhs, e, loc)) return false;
	StructTy *r = as<StructTy>(rhs);
	if(fields.size() != r->fields.size()) {
		e.set(loc, "struct type mismatch (LHS fields: %zu, RHS fields: %zu)", fields.size(),
		      r->fields.size());
		return false;
	}
	for(size_t i = 0; i < fields.size(); ++i) {
		if(fields[i]->isCompatible(r->fields[i], e, loc)) continue;
		e.set(loc, "LHS struct field %s with index %zu, incompatible with RHS field %s",
		      fields[i]->toStr().c_str(), i, r->fields[i]->toStr().c_str());
		return false;
	}
	return true;
}
StructTy *StructTy::instantiate(Context &c, ErrMgr &e, StmtFnCallInfo *callinfo)
{
	if(fields.size() != callinfo->getArgs().size()) return nullptr;
	ModuleLoc &loc		 = callinfo->getLoc();
	bool is_field_compatible = true;
	for(size_t i = 0; i < this->fields.size(); ++i) {
		Type *sf    = this->fields[i];
		Stmt *ciarg = callinfo->getArg(i);
		if(sf->isCompatible(ciarg->getType(), e, loc)) continue;
		is_field_compatible = false;
		break;
	}
	if(!is_field_compatible) return nullptr;
	StructTy *newst = as<StructTy>(this->clone(c));
	newst->setDef(false);
	return newst;
}
StructTy *StructTy::create(Context &c, const std::vector<std::string> &_fieldnames,
			   const std::vector<Type *> &_fields)
{
	return c.allocType<StructTy>(_fieldnames, _fields);
}
std::vector<Type *> &StructTy::getFields()
{
	return fields;
}
Type *StructTy::getField(const std::string &name)
{
	auto found = fieldpos.find(name);
	if(found == fieldpos.end()) return nullptr;
	return fields[found->second];
}
Type *StructTy::getField(const size_t &pos)
{
	if(pos >= fields.size()) return nullptr;
	return fields[pos];
}
void StructTy::setDef(const bool &def)
{
	is_def = def;
}
bool StructTy::isDef() const
{
	return is_def;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Type
///////////////////////////////////////////////////////////////////////////////////////////////////

FuncTy::FuncTy(Stmt *def, const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const bool &externed)
	: Type(TFUNC, 0, genTypeID()), def(def), args(args), ret(ret), intrin(intrin),
	  externed(externed)
{}
FuncTy::FuncTy(Stmt *def, const size_t &info, const uint64_t &id, const std::vector<Type *> &args,
	       Type *ret, IntrinsicFn intrin, const bool &externed)
	: Type(TFUNC, info, id), def(def), args(args), ret(ret), intrin(intrin), externed(externed)
{}
FuncTy::~FuncTy() {}
Type *FuncTy::clone(Context &c)
{
	std::vector<Type *> newargs;
	for(auto &arg : args) newargs.push_back(arg->clone(c));
	return c.allocType<FuncTy>(def, getInfo(), getID(), newargs, ret->clone(c), intrin,
				   externed);
}
std::string FuncTy::toStr()
{
	std::string res = infoToStr() + "function<" + std::to_string(getID());
	if(intrin) res += "intrinsic, ";
	if(externed) res += "extern, ";
	if(intrin || externed) {
		res.pop_back();
		res.pop_back();
	}
	res += ">(";
	for(auto &a : args) {
		res += a->toStr() + ", ";
	}
	if(args.size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += "): " + ret->toStr();
	return res;
}
bool FuncTy::isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(rhs, e, loc)) return false;
	FuncTy *r = as<FuncTy>(rhs);
	if(externed != r->externed) {
		e.set(loc, "func type mismatch (LHS externed: %s, RHS externed: %s)",
		      externed ? "yes" : "no", r->externed ? "yes" : "no");
	}
	if(args.size() != r->args.size()) {
		e.set(loc, "type mismatch (LHS args: %zu, RHS args: %zu", args.size(),
		      r->args.size());
		return false;
	}
	for(size_t i = 0; i < args.size(); ++i) {
		if(args[i]->isCompatible(r->args[i], e, loc)) continue;
		e.set(loc, "LHS function arg %s with index %zu, incompatible with RHS arg %s",
		      args[i]->toStr().c_str(), i, r->args[i]->toStr().c_str());
		return false;
	}
	if(!ret->isCompatible(r->ret, e, loc)) {
		e.set(loc, "incompatible return types (LHS: %s, RHS: %s)", ret->toStr().c_str(),
		      r->ret->toStr().c_str());
		return false;
	}
	return true;
}
// specializes a function type using StmtFnCallInfo
FuncTy *FuncTy::createCall(Context &c, ErrMgr &e, StmtFnCallInfo *callinfo)
{
	if(args.size() != callinfo->getArgs().size()) return nullptr;
	ModuleLoc &loc	       = callinfo->getLoc();
	bool is_arg_compatible = true;
	std::vector<Type *> variadics;
	for(size_t i = 0, j = 0; i < this->args.size() && j < callinfo->getArgs().size(); ++i, ++j)
	{
		Type *sa      = this->args[i];
		Stmt *ciarg   = callinfo->getArg(i);
		bool variadic = false;
		if(sa->hasVariadic()) {
			variadic = true;
			--i;
		}
		if(!sa->isCompatible(ciarg->getType(), e, loc)) {
			is_arg_compatible = false;
			break;
		}
		if(variadic) variadics.push_back(ciarg->getType());
	}
	if(!is_arg_compatible) return nullptr;

	FuncTy *res   = this;
	size_t va_len = res->args.size();
	if(hasVariadic()) {
		res = as<FuncTy>(clone(c));
		--va_len;
		Type *vabase = res->args.back();
		res->args.pop_back();
		vabase->unsetVariadic();
		VariadicTy *va	= VariadicTy::create(c, {});
		size_t ptrcount = getPointerCount(vabase);
		for(auto &vtmp : variadics) {
			Type *v = vtmp->clone(c);
			applyPointerCount(c, v, ptrcount);
			v->appendInfo(vabase->getInfo());
			va->addArg(v);
		}
		res->args.push_back(va);
	}

	return res;
}
FuncTy *FuncTy::create(Context &c, Stmt *_def, const std::vector<Type *> &_args, Type *_ret,
		       IntrinsicFn _intrin, const bool &_externed)
{
	return c.allocType<FuncTy>(_def, _args, _ret, _intrin, _externed);
}
Stmt *&FuncTy::getDef()
{
	return def;
}
std::vector<Type *> &FuncTy::getArgs()
{
	return args;
}
Type *FuncTy::getArg(const size_t &idx)
{
	if(args.size() > idx) return args[idx];
	return nullptr;
}
Type *FuncTy::getRet()
{
	return ret;
}
bool FuncTy::isIntrinsic()
{
	return intrin != nullptr;
}
bool FuncTy::isExtern()
{
	return externed;
}
bool FuncTy::callIntrinsic(Context &c, StmtExpr *stmt, Stmt **source, StmtFnCallInfo *callinfo)
{
	return intrin(c, stmt, source, callinfo);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Variadic Type
///////////////////////////////////////////////////////////////////////////////////////////////////

VariadicTy::VariadicTy(const std::vector<Type *> &args) : Type(TVARIADIC, 0, TVARIADIC), args(args)
{}
VariadicTy::VariadicTy(const size_t &info, const uint64_t &id, const std::vector<Type *> &args)
	: Type(TVARIADIC, info, id), args(args)
{}
VariadicTy::~VariadicTy() {}
Type *VariadicTy::clone(Context &c)
{
	std::vector<Type *> newargs;
	for(auto &arg : args) newargs.push_back(arg->clone(c));
	return c.allocType<VariadicTy>(getInfo(), getID(), newargs);
}
std::string VariadicTy::toStr()
{
	std::string res = infoToStr() + "variadic<";
	for(auto &a : args) {
		res += a->toStr() + ", ";
	}
	if(args.size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += ">";
	return res;
}
bool VariadicTy::isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(rhs, e, loc)) return false;
	VariadicTy *r = as<VariadicTy>(rhs);
	if(args.size() != r->args.size()) return false;
	for(size_t i = 0; i < args.size(); ++i) {
		if(!args[i]->isCompatible(r->args[i], e, loc)) return false;
	}
	return true;
}
VariadicTy *VariadicTy::create(Context &c, const std::vector<Type *> &_args)
{
	return c.allocType<VariadicTy>(_args);
}
void VariadicTy::addArg(Type *ty)
{
	args.push_back(ty);
}
std::vector<Type *> &VariadicTy::getArgs()
{
	return args;
}
Type *VariadicTy::getArg(const size_t &idx)
{
	if(args.size() > idx) return args[idx];
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Import Type
///////////////////////////////////////////////////////////////////////////////////////////////////

ImportTy::ImportTy(const std::string &impid) : Type(TIMPORT, 0, TIMPORT), impid(impid) {}
ImportTy::ImportTy(const size_t &info, const uint64_t &id, const std::string &impid)
	: Type(TIMPORT, info, id), impid(impid)
{}
ImportTy::~ImportTy() {}
Type *ImportTy::clone(Context &c)
{
	return c.allocType<ImportTy>(getInfo(), getID(), impid);
}
std::string ImportTy::toStr()
{
	return infoToStr() + "import<" + impid + ">";
}
ImportTy *ImportTy::create(Context &c, const std::string &_impid)
{
	return c.allocType<ImportTy>(_impid);
}

const std::string &ImportTy::getImportID() const
{
	return impid;
}

size_t getPointerCount(Type *t)
{
	size_t i = 0;
	while(t->isPtr()) {
		++i;
		t = as<PtrTy>(t)->getTo();
	}
	return i;
}
Type *applyPointerCount(Context &c, Type *t, const size_t &count)
{
	for(size_t i = 0; i < count; ++i) {
		t = PtrTy::create(c, t);
	}
	return t;
}
} // namespace sc
