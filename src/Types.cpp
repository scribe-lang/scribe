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
"void",	      "<any>",	  "i1",	    "i8",	  "i16",      "i32",	    "i64",   "u8",
"u16",	      "u32",	  "u64",    "f32",	  "f64",      "<template>", "<ptr>", "<array>",
"<function>", "<struct>", "<enum>", "<variadic>", "<import>", "<funcmap>",
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Base Type
///////////////////////////////////////////////////////////////////////////////////////////////////

Type::Type(const Types &type, const size_t &info, const uint64_t &id)
	: type(type), info(info), id(id)
{}
Type::~Type() {}
bool Type::isPrimitive()
{
	return isInt1() || isInt8() || isInt16() || isInt32() || isInt64() || isUInt8() ||
	       isUInt16() || isUInt32() || isUInt64() || isFlt32() || isFlt64();
}
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
	if(!isTempl() && !num_to_num && getID() != rhs->getID()) {
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
Type *Type::specialize(Context &c, const std::unordered_map<std::string, Type *> &templates,
		       std::unordered_set<std::string> &unresolved_templates)
{
	return this;
}
bool Type::isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	return isBaseCompatible(rhs, e, loc);
}
bool Type::determineTemplateActuals(Context &c, Type *actual, ErrMgr &e, ModuleLoc &loc,
				    std::unordered_map<std::string, Type *> &templates)
{
	if(!isTempl()) return true;
	TemplTy *templ = as<TemplTy>(this);
	auto res       = templates.find(templ->getName());
	if(res != templates.end()) {
		if(!res->second->isCompatible(actual, e, loc)) {
			e.set(loc,
			      "In templated argument, mismatch between template %s and actual %s",
			      res->second->toStr().c_str(), actual->toStr().c_str());
			return false;
		}
		return true;
	}
	if(!isCompatible(actual, e, loc)) {
		e.set(loc, "incompatible actual '%s' to template '%s'", actual->toStr().c_str(),
		      toStr().c_str());
		return false;
	}
	Type *t = actual->clone(c);
	t->setInfo(0);
	while(t->isPtr()) t = as<PtrTy>(t)->getTo(); // remove all pointers(?)
	templates[templ->getName()] = t;
	return true;
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
BasicTypeDefine(Int1Ty, TI1);
BasicTypeDefine(Int8Ty, TI8);
BasicTypeDefine(Int16Ty, TI16);
BasicTypeDefine(Int32Ty, TI32);
BasicTypeDefine(Int64Ty, TI64);
BasicTypeDefine(UInt8Ty, TU8);
BasicTypeDefine(UInt16Ty, TU16);
BasicTypeDefine(UInt32Ty, TU32);
BasicTypeDefine(UInt64Ty, TU64);
BasicTypeDefine(Flt32Ty, TF32);
BasicTypeDefine(Flt64Ty, TF64);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Template Type
///////////////////////////////////////////////////////////////////////////////////////////////////

TemplTy::TemplTy(const std::string &name) : Type(TTEMPL, 0, TTEMPL), name(name) {}
TemplTy::TemplTy(const size_t &info, const uint64_t &id, const std::string &name)
	: Type(TTEMPL, info, id), name(name)
{}
TemplTy::~TemplTy() {}
Type *TemplTy::clone(Context &c)
{
	return c.allocType<TemplTy>(getInfo(), getID(), name);
}
std::string TemplTy::toStr()
{
	return infoToStr() + "@" + name;
}
Type *TemplTy::specialize(Context &c, const std::unordered_map<std::string, Type *> &templates,
			  std::unordered_set<std::string> &unresolved_templates)
{
	Type *res = templates.at(name)->specialize(c, templates, unresolved_templates);
	res->appendInfo(getInfo());
	return res;
}
TemplTy *TemplTy::create(Context &c, const std::string &tname)
{
	return c.allocType<TemplTy>(tname);
}
std::string &TemplTy::getName()
{
	return name;
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
Type *PtrTy::specialize(Context &c, const std::unordered_map<std::string, Type *> &templates,
			std::unordered_set<std::string> &unresolved_templates)
{
	Type *res = to->specialize(c, templates, unresolved_templates);
	if(res == to) return this;
	return c.allocType<PtrTy>(getInfo(), getID(), res);
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
Type *ArrayTy::specialize(Context &c, const std::unordered_map<std::string, Type *> &templates,
			  std::unordered_set<std::string> &unresolved_templates)
{
	Type *resof = of->specialize(c, templates, unresolved_templates);
	if(resof == of) return this;
	return c.allocType<ArrayTy>(getInfo(), getID(), count, resof);
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
bool ArrayTy::determineTemplateActuals(Context &c, Type *actual, ErrMgr &e, ModuleLoc &loc,
				       std::unordered_map<std::string, Type *> &templates)
{
	return of->determineTemplateActuals(c, actual, e, loc, templates);
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

StructTy::StructTy(const std::vector<std::string> &fieldnames, const std::vector<Type *> &fields,
		   const size_t &templs)
	: Type(TSTRUCT, 0, genTypeID()), fields(fields), templs(templs), is_def(true)
{
	for(size_t i = 0; i < fieldnames.size(); ++i) {
		fieldpos[fieldnames[i]] = i;
	}
}
StructTy::StructTy(const size_t &info, const uint64_t &id,
		   const std::unordered_map<std::string, size_t> &fieldpos,
		   const std::vector<Type *> &fields, const size_t &templs, const bool &is_def)
	: Type(TSTRUCT, info, id), fieldpos(fieldpos), fields(fields), is_def(is_def)
{}
StructTy::~StructTy() {}
Type *StructTy::clone(Context &c)
{
	std::vector<Type *> newfields;
	for(auto &field : fields) newfields.push_back(field->clone(c));
	return c.allocType<StructTy>(getInfo(), getID(), fieldpos, newfields, templs, is_def);
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
Type *StructTy::specialize(Context &c, const std::unordered_map<std::string, Type *> &templates,
			   std::unordered_set<std::string> &unresolved_templates)
{
	std::vector<Type *> newfields;
	bool found = false;
	for(auto &f : fields) {
		newfields.push_back(f->specialize(c, templates, unresolved_templates));
		if(newfields.back() == f) continue;
		found = true;
	}
	if(!found) return this;
	return c.allocType<StructTy>(getInfo(), getID(), fieldpos, newfields, templs, is_def);
}
bool StructTy::isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(rhs, e, loc)) return false;
	StructTy *r = as<StructTy>(rhs);
	if(templs != r->templs) {
		e.set(loc, "struct type mismatch (LHS templates: %zu, RHS templates: %zu)", templs,
		      r->templs);
		return false;
	}
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
bool StructTy::determineTemplateActuals(Context &c, Type *actual, ErrMgr &e, ModuleLoc &loc,
					std::unordered_map<std::string, Type *> &templates)
{
	if(!isCompatible(actual, e, loc)) return false;
	StructTy *ast = as<StructTy>(actual);
	for(size_t i = 0; i < fields.size(); ++i) {
		if(!fields[i]->determineTemplateActuals(c, ast->fields[i], e, loc, templates)) {
			return false;
		}
	}
	return true;
}
StructTy *StructTy::instantiate(Context &c, ErrMgr &e, StmtFnCallInfo *callinfo,
				std::unordered_map<std::string, Type *> &templates)
{
	templates.clear();
	if(templs < callinfo->getTemplates().size()) return nullptr;
	if(fields.size() != callinfo->getArgs().size()) return nullptr;
	std::unordered_set<std::string> unresolved_templates;
	for(size_t i = 0; i < templs; ++i) unresolved_templates.insert(std::to_string(i));
	for(size_t i = 0; i < callinfo->getArgs().size(); ++i) {
		std::string istr = std::to_string(i);
		templates[istr]	 = callinfo->getTemplate(i)->getType();
		unresolved_templates.erase(istr);
	}
	ModuleLoc &loc = callinfo->getLoc();
	for(size_t i = 0; i < fields.size(); ++i) {
		Type *callargty = callinfo->getArg(i)->getType();
		if(fields[i]->determineTemplateActuals(c, callargty, e, loc, templates)) continue;
		return nullptr;
	}
	bool is_field_compatible = true;
	std::vector<Type *> specializedfields;
	for(auto &f : this->fields) {
		specializedfields.push_back(f->specialize(c, templates, unresolved_templates));
	}
	for(size_t i = 0; i < specializedfields.size(); ++i) {
		Type *sf    = specializedfields[i];
		Stmt *ciarg = callinfo->getArg(i);
		if(sf->isCompatible(ciarg->getType(), e, loc)) continue;
		is_field_compatible = false;
		break;
	}
	if(!is_field_compatible) return nullptr;
	StructTy *newst = as<StructTy>(specialize(c, templates, unresolved_templates));
	if(!unresolved_templates.empty()) {
		e.set(loc, "failed to instantiate struct - not all templates were resolved");
		return nullptr;
	}
	newst->setDef(false);
	return newst;
}
StructTy *StructTy::create(Context &c, const std::vector<std::string> &_fieldnames,
			   const std::vector<Type *> &_fields, const size_t &templs)
{
	return c.allocType<StructTy>(_fieldnames, _fields, templs);
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
void StructTy::setTemplates(const size_t &_templs)
{
	templs = _templs;
}
void StructTy::setDef(const bool &def)
{
	is_def = def;
}
bool StructTy::isDef() const
{
	return is_def;
}
const size_t &StructTy::getTemplates() const
{
	return templs;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Type
///////////////////////////////////////////////////////////////////////////////////////////////////

FuncTy::FuncTy(Stmt *def, const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const size_t &templs, const bool &externed)
	: Type(TFUNC, 0, genTypeID()), def(def), args(args), ret(ret), intrin(intrin),
	  templs(templs), externed(externed)
{}
FuncTy::FuncTy(Stmt *def, const size_t &info, const uint64_t &id, const std::vector<Type *> &args,
	       Type *ret, IntrinsicFn intrin, const size_t &templs, const bool &externed)
	: Type(TFUNC, info, id), def(def), args(args), ret(ret), intrin(intrin), templs(templs),
	  externed(externed)
{}
FuncTy::~FuncTy() {}
Type *FuncTy::clone(Context &c)
{
	std::vector<Type *> newargs;
	for(auto &arg : args) newargs.push_back(arg->clone(c));
	return c.allocType<FuncTy>(getInfo(), getID(), def, newargs, ret->clone(c), intrin);
}
std::string FuncTy::toStr()
{
	std::string res = infoToStr() + "function<" + std::to_string(getID());
	if(intrin) res += "intrinsic, ";
	if(templs) res += "templates: " + std::to_string(templs) + ", ";
	if(externed) res += "extern, ";
	if(intrin || templs || externed) {
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
Type *FuncTy::specialize(Context &c, const std::unordered_map<std::string, Type *> &templates,
			 std::unordered_set<std::string> &unresolved_templates)
{
	std::vector<Type *> newargs;
	bool found = false;
	for(auto &a : args) {
		newargs.push_back(a->specialize(c, templates, unresolved_templates));
		if(newargs.back() == a) continue;
		found = true;
	}
	Type *newret = ret->specialize(c, templates, unresolved_templates);
	if(!found && newret == ret) return this;
	return c.allocType<FuncTy>(getInfo(), getID(), def, newargs, newret, externed);
}
bool FuncTy::isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(rhs, e, loc)) return false;
	FuncTy *r = as<FuncTy>(rhs);
	if(templs != r->templs) {
		e.set(loc, "func type mismatch (LHS templates: %zu, RHS templates: %zu)", templs,
		      r->templs);
		return false;
	}
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
bool FuncTy::determineTemplateActuals(Context &c, Type *actual, ErrMgr &e, ModuleLoc &loc,
				      std::unordered_map<std::string, Type *> &templates)
{
	if(!isCompatible(actual, e, loc)) return false;
	FuncTy *afn = as<FuncTy>(actual);
	for(size_t i = 0; i < args.size(); ++i) {
		if(!args[i]->determineTemplateActuals(c, afn->args[i], e, loc, templates)) {
			return false;
		}
	}
	if(!ret->determineTemplateActuals(c, afn->ret, e, loc, templates)) return false;
	return true;
}
// specializes a function type using StmtFnCallInfo
FuncTy *FuncTy::createCall(Context &c, ErrMgr &e, StmtFnCallInfo *callinfo,
			   std::unordered_map<std::string, Type *> &templates)
{
	templates.clear();
	if(templs < callinfo->getTemplates().size()) return nullptr;
	if(args.size() != callinfo->getArgs().size()) return nullptr;
	std::unordered_set<std::string> unresolved_templates;
	for(size_t i = 0; i < templs; ++i) unresolved_templates.insert(std::to_string(i));
	for(size_t i = 0; i < callinfo->getArgs().size(); ++i) {
		std::string istr = std::to_string(i);
		templates[istr]	 = callinfo->getTemplate(i)->getType();
		unresolved_templates.erase(istr);
	}
	ModuleLoc &loc = callinfo->getLoc();
	for(size_t i = 0; i < args.size(); ++i) {
		Type *callargty = callinfo->getArg(i)->getType();
		if(args[i]->determineTemplateActuals(c, callargty, e, loc, templates)) continue;
		return nullptr;
	}
	bool is_arg_compatible = true;
	std::vector<Type *> specializedargs;
	for(auto &a : this->args) {
		specializedargs.push_back(a->specialize(c, templates, unresolved_templates));
	}
	std::vector<Type *> variadics;
	for(size_t i = 0, j = 0; i < specializedargs.size() && j < callinfo->getArgs().size();
	    ++i, ++j) {
		Type *sa      = specializedargs[i];
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

	FuncTy *tmp   = this;
	size_t va_len = tmp->args.size();
	if(hasVariadic()) {
		tmp = as<FuncTy>(clone(c));
		--va_len;
		Type *vabase = tmp->args.back();
		tmp->args.pop_back();
		vabase->unsetVariadic();
		VariadicTy *va	= VariadicTy::create(c, {});
		size_t ptrcount = getPointerCount(vabase);
		for(auto &vtmp : variadics) {
			Type *v = vtmp->clone(c);
			applyPointerCount(c, v, ptrcount);
			v->appendInfo(vabase->getInfo());
			va->addArg(v);
		}
		tmp->args.push_back(va);
	}

	FuncTy *res = as<FuncTy>(tmp->specialize(c, templates, unresolved_templates));
	if(!unresolved_templates.empty()) {
		e.set(loc, "failed to create function call - not all templates were resolved");
		return nullptr;
	}
	return res;
}
FuncTy *FuncTy::create(Context &c, Stmt *_def, const std::vector<Type *> &_args, Type *_ret,
		       IntrinsicFn _intrin, const size_t &_templs, const bool &_externed)
{
	return c.allocType<FuncTy>(_def, _args, _ret, _intrin, _templs, _externed);
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
Type *VariadicTy::specialize(Context &c, const std::unordered_map<std::string, Type *> &templates,
			     std::unordered_set<std::string> &unresolved_templates)
{
	std::vector<Type *> newargs;
	bool found = false;
	for(auto &a : args) {
		newargs.push_back(a->specialize(c, templates, unresolved_templates));
		if(newargs.back() == a) continue;
		found = true;
	}
	if(!found) return this;
	return c.allocType<VariadicTy>(getInfo(), getID(), newargs);
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
bool VariadicTy::determineTemplateActuals(Context &c, Type *actual, ErrMgr &e, ModuleLoc &loc,
					  std::unordered_map<std::string, Type *> &templates)
{
	if(!isCompatible(actual, e, loc)) return false;
	VariadicTy *ava = as<VariadicTy>(actual);
	for(size_t i = 0; i < args.size(); ++i) {
		if(!args[i]->determineTemplateActuals(c, ava->args[i], e, loc, templates)) {
			return false;
		}
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
