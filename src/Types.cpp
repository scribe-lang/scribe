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
#include "Values.hpp"

namespace sc
{
static const char *TypeStrs[] = {
"void",	      "<any>",	  "int",    "flt",	  "<template>", "<ptr>",     "<array>",
"<function>", "<struct>", "<enum>", "<variadic>", "<import>",	"<funcmap>",
};
static std::unordered_map<uint64_t, Type *> containedtypes;

uint64_t genTypeID()
{
	static uint64_t id = _LAST;
	return id++;
}
uint64_t genFuncUniqID()
{
	static uint64_t id = 1; // 0 is for externs = no uniq id
	return id++;
}
uint64_t genContainedTypeID()
{
	static uint64_t id = 0;
	return id++;
}

size_t getPointerCount(Type *t);
Type *applyPointerCount(Context &c, Type *t, const size_t &count);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Base Type
///////////////////////////////////////////////////////////////////////////////////////////////////

Type::Type(const Types &type, const size_t &info, const uint64_t &id)
	: type(type), info(info), id(id)
{}
Type::~Type() {}
bool Type::isBaseCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!rhs) return false;
	if(isAny()) return true;
	if(isPtr() && rhs->isPtr()) {
		return as<PtrTy>(this)->getTo()->isCompatible(c, as<PtrTy>(rhs)->getTo(), e, loc);
	}
	if(rhs->isTypeTy()) {
		return this->isCompatible(c, as<TypeTy>(rhs)->getContainedTy(), e, loc);
	}
	if(isTypeTy()) {
		Type *ct = as<TypeTy>(this)->getContainedTy();
		if(!ct) return true;
		return ct->isCompatible(c, rhs, e, loc);
	}
	bool is_lhs_prim = isPrimitive();
	bool is_rhs_prim = rhs->isPrimitive();
	size_t lhs_ptr	 = getPointerCount(this);
	size_t rhs_ptr	 = getPointerCount(rhs);
	bool num_to_num	 = false;
	if(!lhs_ptr && !rhs_ptr) {
		num_to_num = is_lhs_prim && is_rhs_prim;
	}
	if(!num_to_num && !(lhs_ptr && is_rhs_prim) && getID() != rhs->getID()) {
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
	if(rhs->hasConst() && !hasConst() && (isPtr() || hasRef())) {
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
	if(info & COMPTIME) res += "comptime ";
	if(info & VARIADIC) res += "...";
	return res;
}
std::string Type::baseToStr()
{
	return infoToStr() + TypeStrs[type];
}
bool Type::requiresCast(Type *other)
{
	if(!isPrimitiveOrPtr() || !other->isPrimitiveOrPtr()) return false;
	if(isPtr() && other->isPtr()) {
		return as<PtrTy>(this)->getTo()->requiresCast(as<PtrTy>(other)->getTo());
	}

	if(!isPrimitive() || !other->isPrimitive()) return getID() != other->getID();
	if(getID() != other->getID()) return true;
	// since id matching is done, both must be of same type only
	if(isInt() && other->isInt()) {
		return as<IntTy>(this)->getBits() != as<IntTy>(other)->getBits() ||
		       as<IntTy>(this)->isSigned() != as<IntTy>(other)->isSigned();
	}
	if(isFlt() && other->isFlt()) {
		return as<FltTy>(this)->getBits() != as<FltTy>(other)->getBits();
	}
	return false;
}
uint64_t Type::getID()
{
	return getBaseID();
}
bool Type::isTemplate()
{
	return false;
}
std::string Type::toStr()
{
	return baseToStr();
}
bool Type::isCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	return isBaseCompatible(c, rhs, e, loc);
}

Value *Type::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	e.set(loc, "invalid type for toDefaultValue(): %s", toStr().c_str());
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Void Type
///////////////////////////////////////////////////////////////////////////////////////////////////

VoidTy::VoidTy() : Type(TVOID, 0, TVOID) {}
VoidTy::VoidTy(const size_t &info) : Type(TVOID, info, TVOID) {}
VoidTy::~VoidTy() {}
Type *VoidTy::clone(Context &c, const bool &as_is)
{
	return c.allocType<VoidTy>(getInfo());
}
std::string VoidTy::toStr()
{
	return "void";
}
VoidTy *VoidTy::create(Context &c)
{
	return c.allocType<VoidTy>();
}
Value *VoidTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	return VoidVal::create(c);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Any Type
///////////////////////////////////////////////////////////////////////////////////////////////////

AnyTy::AnyTy() : Type(TANY, 0, TANY) {}
AnyTy::AnyTy(const size_t &info) : Type(TANY, info, TANY) {}
AnyTy::~AnyTy() {}
Type *AnyTy::clone(Context &c, const bool &as_is)
{
	return c.allocType<AnyTy>(getInfo());
}
std::string AnyTy::toStr()
{
	return "any";
}
AnyTy *AnyTy::create(Context &c)
{
	return c.allocType<AnyTy>();
}

Value *AnyTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	return TypeVal::create(c, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Int Type
///////////////////////////////////////////////////////////////////////////////////////////////////

IntTy::IntTy(const size_t &bits, const bool &sign) : Type(TINT, 0, TINT), bits(bits), sign(sign) {}
IntTy::IntTy(const size_t &info, const uint64_t &id, const size_t &bits, const bool &sign)
	: Type(TINT, info, id), bits(bits), sign(sign)
{}
IntTy::~IntTy() {}

uint64_t IntTy::getID()
{
	return getBaseID() + bits + (sign * 2);
}
Type *IntTy::clone(Context &c, const bool &as_is)
{
	return c.allocType<IntTy>(getInfo(), getBaseID(), bits, sign);
}
std::string IntTy::toStr()
{
	return infoToStr() + (sign ? "i" : "u") + std::to_string(bits);
}

IntTy *IntTy::create(Context &c, const size_t &_bits, const bool &_sign)
{
	return c.allocType<IntTy>(_bits, _sign);
}

Value *IntTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	return IntVal::create(c, this, cd, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Float Type
///////////////////////////////////////////////////////////////////////////////////////////////////

FltTy::FltTy(const size_t &bits) : Type(TFLT, 0, TFLT), bits(bits) {}
FltTy::FltTy(const size_t &info, const uint64_t &id, const size_t &bits)
	: Type(TFLT, info, id), bits(bits)
{}
FltTy::~FltTy() {}

uint64_t FltTy::getID()
{
	return getBaseID() + (bits * 3); // * 3 to prevent clash between int and flt
}
Type *FltTy::clone(Context &c, const bool &as_is)
{
	return c.allocType<FltTy>(getInfo(), getBaseID(), bits);
}
std::string FltTy::toStr()
{
	return infoToStr() + "f" + std::to_string(bits);
}

FltTy *FltTy::create(Context &c, const size_t &_bits)
{
	return c.allocType<FltTy>(_bits);
}

Value *FltTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	return FltVal::create(c, this, cd, 0.0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Type Type
///////////////////////////////////////////////////////////////////////////////////////////////////

TypeTy::TypeTy() : Type(TTYPE, 0, TTYPE), containedtyid(genContainedTypeID()) {}
TypeTy::TypeTy(const size_t &info, const uint64_t &id, const uint64_t &containedtyid)
	: Type(TTYPE, info, id), containedtyid(containedtyid)
{}
TypeTy::~TypeTy() {}

bool TypeTy::isTemplate()
{
	return !getContainedTy();
}
std::string TypeTy::toStr()
{
	Type *ct = getContainedTy();
	return infoToStr() + "typety<" +
	       (ct ? ct->toStr() : "(none:" + std::to_string(containedtyid) + ")") + ">";
}
Type *TypeTy::clone(Context &c, const bool &as_is)
{
	if(!as_is && getContainedTy()) return getContainedTy()->clone(c, as_is);
	return c.allocType<TypeTy>(getInfo(), getBaseID(), containedtyid);
}

TypeTy *TypeTy::create(Context &c)
{
	return c.allocType<TypeTy>();
}

void TypeTy::clearContainedTy()
{
	containedtypes[containedtyid] = nullptr;
}
void TypeTy::setContainedTy(Type *ty)
{
	if(getContainedTy()) return;
	if(ty->isTypeTy() && as<TypeTy>(ty)->getContainedTy()) {
		containedtypes[containedtyid] = as<TypeTy>(ty)->getContainedTy();
		return;
	}
	containedtypes[containedtyid] = ty;
}
Type *TypeTy::getContainedTy()
{
	auto loc = containedtypes.find(containedtyid);
	if(loc == containedtypes.end()) return nullptr;
	return loc->second;
}

Value *TypeTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	if(!getContainedTy()) {
		return TypeVal::create(c, this);
	}
	return getContainedTy()->toDefaultValue(c, e, loc, cd);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Pointer Type
///////////////////////////////////////////////////////////////////////////////////////////////////

PtrTy::PtrTy(Type *to, const size_t &count) : Type(TPTR, 0, TPTR), to(to), count(count) {}
PtrTy::PtrTy(const size_t &info, const uint64_t &id, Type *to, const size_t &count)
	: Type(TPTR, info, id), to(to), count(count)
{}
PtrTy::~PtrTy() {}
bool PtrTy::isTemplate()
{
	return to->isTemplate();
}
std::string PtrTy::toStr()
{
	std::string countexpr;
	if(count) countexpr = "[" + std::to_string(count) + "] ";
	return "*" + countexpr + infoToStr() + to->toStr();
}
Type *PtrTy::clone(Context &c, const bool &as_is)
{
	return c.allocType<PtrTy>(getInfo(), getBaseID(), to->clone(c, as_is), count);
}
PtrTy *PtrTy::create(Context &c, Type *ptr_to, const size_t &count)
{
	return c.allocType<PtrTy>(ptr_to, count);
}

Value *PtrTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	std::vector<Value *> vec;
	Value *res = to->toDefaultValue(c, e, loc, cd);
	if(!res) {
		e.set(loc, "failed to get default value from array's type");
		return nullptr;
	}
	vec.push_back(res);
	for(size_t i = 1; i < count; ++i) {
		vec.push_back(res->clone(c));
	}
	return VecVal::create(c, this, cd, vec);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Struct Type
///////////////////////////////////////////////////////////////////////////////////////////////////

StructTy::StructTy(const std::vector<std::string> &fieldnames, const std::vector<Type *> &fields,
		   const std::vector<std::string> &templatenames,
		   const std::vector<TypeTy *> &templates, const bool &externed)
	: Type(TSTRUCT, 0, genTypeID()), fieldnames(fieldnames), fields(fields),
	  templatenames(templatenames), templates(templates), has_template(templates.size()),
	  externed(externed)
{
	for(size_t i = 0; i < fieldnames.size(); ++i) {
		fieldpos[fieldnames[i]] = i;
	}
	for(size_t i = 0; i < templatenames.size(); ++i) {
		templatepos[templatenames[i]] = i;
	}
}
StructTy::StructTy(const size_t &info, const uint64_t &id,
		   const std::vector<std::string> &fieldnames,
		   const std::unordered_map<std::string, size_t> &fieldpos,
		   const std::vector<Type *> &fields, const std::vector<std::string> &templatenames,
		   const std::unordered_map<std::string, size_t> &templatepos,
		   const std::vector<TypeTy *> &templates, const bool &has_template,
		   const bool &externed)
	: Type(TSTRUCT, info, id), fieldpos(fieldpos), fieldnames(fieldnames), fields(fields),
	  templatepos(templatepos), templatenames(templatenames), templates(templates),
	  has_template(has_template), externed(externed)
{}
StructTy::~StructTy() {}
bool StructTy::isTemplate()
{
	for(auto &f : fields) {
		if(f->isTemplate()) return true;
	}
	return false;
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
Type *StructTy::clone(Context &c, const bool &as_is)
{
	std::vector<Type *> newfields;
	std::vector<TypeTy *> newtemplates;
	for(auto &field : fields) newfields.push_back(field->clone(c, as_is));
	for(auto &t : templates) newtemplates.push_back(as<TypeTy>(t->clone(c, as_is)));
	return c.allocType<StructTy>(getInfo(), getBaseID(), fieldnames, fieldpos, newfields,
				     templatenames, templatepos, newtemplates, has_template,
				     externed);
}
bool StructTy::isCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(c, rhs, e, loc)) return false;
	StructTy *r = as<StructTy>(rhs);
	if(fields.size() != r->fields.size()) {
		e.set(loc, "struct type mismatch (LHS fields: %zu, RHS fields: %zu)", fields.size(),
		      r->fields.size());
		return false;
	}
	for(size_t i = 0; i < fields.size(); ++i) {
		if(fields[i]->isCompatible(c, r->fields[i], e, loc)) continue;
		e.set(loc, "LHS struct field %s with index %zu, incompatible with RHS field %s",
		      fields[i]->toStr().c_str(), i, r->fields[i]->toStr().c_str());
		return false;
	}
	return true;
}
StructTy *StructTy::applyTemplates(Context &c, ErrMgr &e, ModuleLoc &loc,
				   const std::vector<Type *> &actuals)
{
	if(templates.size() != actuals.size()) {
		e.set(loc, "expected templates for struct: %zu, found: %zu", templates.size(),
		      actuals.size());
		return nullptr;
	}
	for(size_t i = 0; i < templates.size(); ++i) {
		templates[i]->setContainedTy(actuals[i]);
	}
	StructTy *res = as<StructTy>(this->clone(c));
	for(auto &t : templates) {
		t->clearContainedTy();
	}
	res->setTemplate(false);
	return res;
}
StructTy *StructTy::instantiate(Context &c, ErrMgr &e, ModuleLoc &loc,
				const std::vector<Stmt *> &callargs)
{
	if(fields.size() != callargs.size()) return nullptr;
	if(isTemplate()) {
		e.set(loc, "a struct with templates cannot be instantiated");
		return nullptr;
	}
	bool is_field_compatible = true;
	for(size_t i = 0; i < this->fields.size(); ++i) {
		Type *sf    = this->fields[i];
		Stmt *ciarg = callargs[i];
		if(sf->isCompatible(c, ciarg->getValueTy(), e, loc)) continue;
		is_field_compatible = false;
		break;
	}
	if(!is_field_compatible) return nullptr;
	StructTy *newst = as<StructTy>(this->clone(c));
	return newst;
}
StructTy *StructTy::create(Context &c, const std::vector<std::string> &_fieldnames,
			   const std::vector<Type *> &_fields,
			   const std::vector<std::string> &_templatenames,
			   const std::vector<TypeTy *> &_templates, const bool &_externed)
{
	return c.allocType<StructTy>(_fieldnames, _fields, _templatenames, _templates, _externed);
}
Type *StructTy::getField(const std::string &name)
{
	auto templfound = templatepos.find(name);
	if(templfound != templatepos.end()) return templates[templfound->second];
	auto found = fieldpos.find(name);
	if(found == fieldpos.end()) return nullptr;
	return fields[found->second];
}
Type *StructTy::getField(const size_t &pos)
{
	if(pos >= fields.size()) return nullptr;
	return fields[pos];
}
bool StructTy::hasTemplate()
{
	if(!has_template) return false;
	for(auto &t : templates) {
		if(!t->getContainedTy()) return true;
	}
	return false;
}

Value *StructTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	std::unordered_map<std::string, Value *> st;
	for(auto &f : fieldpos) {
		Value *res = fields[f.second]->toDefaultValue(c, e, loc, cd);
		if(!res) {
			e.set(loc, "failed to get default value from array's type");
			return nullptr;
		}
		st[f.first] = res;
	}
	return StructVal::create(c, this, cd, st);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Type
///////////////////////////////////////////////////////////////////////////////////////////////////

FuncTy::FuncTy(StmtVar *var, const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const IntrinType &inty, const bool &externed)
	: Type(TFUNC, 0, genTypeID()), var(var), args(args), ret(ret), intrin(intrin), inty(inty),
	  uniqid(!externed ? genFuncUniqID() : 0), externed(externed)
{}
FuncTy::FuncTy(const size_t &info, const uint64_t &id, StmtVar *var,
	       const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const IntrinType &inty, const uint64_t &uniqid, const bool &externed)
	: Type(TFUNC, info, id), var(var), args(args), ret(ret), intrin(intrin), inty(inty),
	  uniqid(uniqid), externed(externed)
{}
FuncTy::~FuncTy() {}
uint64_t FuncTy::getID()
{
	uint64_t res = getBaseID() + uniqid;
	for(auto &a : args) {
		res += a->getID();
	}
	res += ret->getID();
	return res * 7;
}
bool FuncTy::isTemplate()
{
	for(auto &a : args) {
		if(a->isTemplate()) return true;
	}
	return ret->isTemplate();
}
std::string FuncTy::toStr()
{
	std::string res = infoToStr() + "function<" + std::to_string(getID());
	if(intrin || externed) res += ", ";
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
Type *FuncTy::clone(Context &c, const bool &as_is)
{
	std::vector<Type *> newargs;
	for(auto &arg : args) newargs.push_back(arg->clone(c, as_is));
	return c.allocType<FuncTy>(getInfo(), getBaseID(), var, newargs, ret->clone(c, as_is),
				   intrin, inty, uniqid, externed);
}
bool FuncTy::isCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(c, rhs, e, loc)) return false;
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
		if(args[i]->isCompatible(c, r->args[i], e, loc)) continue;
		e.set(loc, "LHS function arg %s with index %zu, incompatible with RHS arg %s",
		      args[i]->toStr().c_str(), i, r->args[i]->toStr().c_str());
		return false;
	}
	if(!ret->isCompatible(c, r->ret, e, loc)) {
		e.set(loc, "incompatible return types (LHS: %s, RHS: %s)", ret->toStr().c_str(),
		      r->ret->toStr().c_str());
		return false;
	}
	return true;
}
// specializes a function type using StmtFnCallInfo
FuncTy *FuncTy::createCall(Context &c, ErrMgr &e, ModuleLoc &loc,
			   const std::vector<Stmt *> &callargs)
{
	if(args.size() > callargs.size()) return nullptr;
	bool has_va = false;
	if(!args.empty() && args.back()->hasVariadic()) has_va = true;
	if(args.size() != callargs.size() && !has_va) return nullptr;
	bool is_arg_compatible = true;
	std::vector<Type *> variadics;
	bool has_templ = false;
	for(size_t i = 0; i < args.size(); ++i) {
		if(!args[i]->isTypeTy()) continue;
		as<TypeTy>(args[i])->setContainedTy(callargs[i]->getValueTy());
		has_templ = true;
	}
	for(size_t i = 0, j = 0; i < this->args.size() && j < callargs.size(); ++i, ++j) {
		Type *sa      = this->args[i];
		Stmt *ciarg   = callargs[j];
		bool variadic = false;
		if(sa->hasVariadic()) {
			variadic = true;
			--i;
		}
		if(!sa->isCompatible(c, ciarg->getValueTy(), e, loc)) {
			is_arg_compatible = false;
			break;
		}
		if(variadic) variadics.push_back(ciarg->getValueTy());
	}
	if(!is_arg_compatible) return nullptr;

	FuncTy *res   = this;
	size_t va_len = res->args.size();
	if(!variadics.empty()) {
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
		has_templ = true;
	}
	res = as<FuncTy>(res->clone(c));
	for(size_t i = 0; i < args.size(); ++i) {
		if(args[i]->isTypeTy()) {
			as<TypeTy>(args[i])->clearContainedTy();
			continue;
		}
	}
	if(has_templ) res->updateUniqID();
	return res;
}
FuncTy *FuncTy::create(Context &c, StmtVar *_var, const std::vector<Type *> &_args, Type *_ret,
		       IntrinsicFn _intrin, const IntrinType &_inty, const bool &_externed)
{
	return c.allocType<FuncTy>(_var, _args, _ret, _intrin, _inty, _externed);
}
void FuncTy::updateUniqID()
{
	uniqid = genFuncUniqID();
}
bool FuncTy::callIntrinsic(Context &c, ErrMgr &err, StmtExpr *stmt, Stmt **source,
			   std::vector<Stmt *> &callargs)
{
	return intrin(c, err, stmt, source, callargs);
}
Value *FuncTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	return FuncVal::create(c, this);
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
bool VariadicTy::isTemplate()
{
	for(auto &a : args) {
		if(a->isTemplate()) return true;
	}
	return false;
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
Type *VariadicTy::clone(Context &c, const bool &as_is)
{
	std::vector<Type *> newargs;
	for(auto &arg : args) newargs.push_back(arg->clone(c, as_is));
	return c.allocType<VariadicTy>(getInfo(), getBaseID(), newargs);
}
bool VariadicTy::isCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc)
{
	if(!isBaseCompatible(c, rhs, e, loc)) return false;
	VariadicTy *r = as<VariadicTy>(rhs);
	if(args.size() != r->args.size()) return false;
	for(size_t i = 0; i < args.size(); ++i) {
		if(!args[i]->isCompatible(c, r->args[i], e, loc)) return false;
	}
	return true;
}
VariadicTy *VariadicTy::create(Context &c, const std::vector<Type *> &_args)
{
	return c.allocType<VariadicTy>(_args);
}
Value *VariadicTy::toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd)
{
	std::vector<Value *> vec;
	for(auto &a : args) {
		Value *v = a->toDefaultValue(c, e, loc, cd);
		if(!v) {
			e.set(loc, "failed to generate default value for type: %s",
			      a->toStr().c_str());
			return nullptr;
		}
		vec.push_back(v);
	}
	return VecVal::create(c, this, cd, vec);
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
		t = PtrTy::create(c, t, 0);
	}
	return t;
}
} // namespace sc
