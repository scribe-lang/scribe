#include "Types.hpp"

#include "Parser/Stmts.hpp"
#include "Values.hpp"

#define MAX_WEAKPTR_DEPTH 7

namespace sc
{
static constexpr uint32_t MAX_FLT_ID = TFLT + 128 * 3;

static const char *TypeStrs[] = {
"void",	      "<any>",	  "int",    "flt",	  "<template>", "<ptr>",     "<array>",
"<function>", "<struct>", "<enum>", "<variadic>", "<import>",	"<funcmap>",
};
static Map<uint32_t, Type *> containedtypes;

static StructTy *strrefty = nullptr;

uint32_t genTypeID()
{
	static uint32_t id = MAX_FLT_ID + 1;
	return id++;
}
uint32_t genFuncUniqID()
{
	static uint32_t id = 1; // 0 is for externs = no uniq id
	return id++;
}
uint32_t genContainedTypeID()
{
	static uint32_t id = 0;
	return id++;
}

size_t getPointerCount(Type *t);
Type *applyPointerCount(Context &c, Type *t, uint16_t count);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Base Type
///////////////////////////////////////////////////////////////////////////////////////////////////

Type::Type(const Types &type) : type(type) {}
Type::~Type() {}
bool Type::isBaseCompatible(Context &c, Type *rhs, const ModuleLoc *loc)
{
	if(!rhs) return false;
	if(isAny()) return true;
	if(isFunc() && rhs->isFunc()) {
		return as<FuncTy>(this)->getSignatureID() == as<FuncTy>(rhs)->getSignatureID();
	}
	if(isPtr() && rhs->isPtr()) {
		if(as<PtrTy>(this)->isWeak() || as<PtrTy>(rhs)->isWeak()) {
			Type *lto = as<PtrTy>(this)->getTo();
			Type *rto = as<PtrTy>(rhs)->getTo();
			while(lto->isTypeTy()) lto = as<TypeTy>(lto)->getContainedTy();
			return lto->getID() == rto->getID();
		}
		return as<PtrTy>(this)->getTo()->isCompatible(c, as<PtrTy>(rhs)->getTo(), loc);
	}
	// useful for passing functions with templates as arguments (callbacks)
	if(isTypeTy() && rhs->isTypeTy()) {
		if(!as<TypeTy>(this)->getContainedTy() && !as<TypeTy>(rhs)->getContainedTy()) {
			// return true;
			err::out(loc, "both typetys contain no type - currently unsupported");
			return false;
		}
	}
	if(rhs->isTypeTy()) {
		return this->isCompatible(c, as<TypeTy>(rhs)->getContainedTy(), loc);
	}
	if(isTypeTy()) {
		Type *ct = as<TypeTy>(this)->getContainedTy();
		if(!ct) return true;
		return ct->isCompatible(c, rhs, loc);
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
		err::out(loc, "different type ids (LHS: ", toStr(), ", RHS: ", rhs->toStr(),
			 ") not compatible");
		return false;
	}
	if(!lhs_ptr && rhs_ptr) {
		err::out(loc, "cannot use a pointer type (LHS: ", rhs->toStr(),
			 ") against non pointer (RHS: ", toStr(), ")");
		return false;
	}
	if(!rhs_ptr && lhs_ptr && !is_rhs_prim) {
		err::out(loc, "non pointer type (RHS: ", rhs->toStr(),
			 ") cannot be assigned to pointer type (LHS: ", toStr(), ")");
		return false;
	}
	if(rhs_ptr != lhs_ptr && !is_rhs_prim) {
		err::out(loc, "inequal pointer assignment here (LHS: ", toStr(),
			 ", RHS: ", rhs->toStr(), ")");
		return false;
	}
	return true;
}
String Type::baseToStr() { return TypeStrs[type]; }
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
		if(!as<IntTy>(this)->getBits()) return false;
		return as<IntTy>(this)->getBits() != as<IntTy>(other)->getBits() ||
		       as<IntTy>(this)->isSigned() != as<IntTy>(other)->isSigned();
	}
	if(isFlt() && other->isFlt()) {
		if(!as<FltTy>(this)->getBits()) return false;
		return as<FltTy>(this)->getBits() != as<FltTy>(other)->getBits();
	}
	return false;
}
uint32_t Type::getUniqID() { return getID(); }
uint32_t Type::getID() { return getBaseID(); }
bool Type::isTemplate(size_t weak_depth) { return false; }
String Type::toStr(size_t weak_depth) { return baseToStr(); }
bool Type::isCompatible(Context &c, Type *rhs, const ModuleLoc *loc)
{
	return isBaseCompatible(c, rhs, loc);
}
bool Type::mergeTemplatesFrom(Type *ty, size_t weak_depth) { return false; }
void Type::unmergeTemplates(size_t weak_depth) {}

bool Type::isStrLiteral()
{
	if(!isPtr()) return false;
	PtrTy *pty = as<PtrTy>(this);
	if(!pty->getTo() || !pty->getTo()->isInt()) return false;
	IntTy *toty = as<IntTy>(pty->getTo());
	if(!toty->isSigned() || toty->getBits() != 8) return false;
	return true;
}
bool Type::isStrRef()
{
	if(!isStruct()) return false;
	StructTy *sty = as<StructTy>(this);
	if(sty->getFields().size() != 2) return false;
	if(sty->getFieldName(0) != "data" || sty->getFieldName(1) != "length") return false;
	if(!sty->getField(0)->isStrLiteral() || !sty->getField(1)->isIntegral()) return false;
	IntTy *ity = as<IntTy>(sty->getField(1));
	if(ity->getBits() != 64 || ity->isSigned()) return false;
	return true;
}

Value *Type::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd, size_t weak_depth)
{
	err::out(loc, "invalid type for toDefaultValue(): ", toStr());
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Void Type
///////////////////////////////////////////////////////////////////////////////////////////////////

VoidTy::VoidTy() : Type(TVOID) {}
VoidTy::~VoidTy() {}
Type *VoidTy::specialize(Context &c, size_t weak_depth) { return this; }
String VoidTy::toStr(size_t weak_depth) { return "void"; }
VoidTy *VoidTy::get(Context &c)
{
	static VoidTy *res = c.allocType<VoidTy>();
	return res;
}
Value *VoidTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd, size_t weak_depth)
{
	err::out(loc, "void type has no value");
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Any Type
///////////////////////////////////////////////////////////////////////////////////////////////////

AnyTy::AnyTy() : Type(TANY) {}
AnyTy::~AnyTy() {}
Type *AnyTy::specialize(Context &c, size_t weak_depth) { return this; }
String AnyTy::toStr(size_t weak_depth) { return "any"; }
AnyTy *AnyTy::get(Context &c)
{
	static AnyTy *res = c.allocType<AnyTy>();
	return res;
}

Value *AnyTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd, size_t weak_depth)
{
	return TypeVal::create(c, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Int Type
///////////////////////////////////////////////////////////////////////////////////////////////////

IntTy::IntTy(uint16_t bits, bool sign) : Type(TINT), bits(bits), sign(sign) {}
IntTy::~IntTy() {}

Type *IntTy::specialize(Context &c, size_t weak_depth) { return this; }
uint32_t IntTy::getID() { return getBaseID() + bits + (sign * 2); }
String IntTy::toStr(size_t weak_depth) { return (sign ? "i" : "u") + std::to_string(bits); }

IntTy *IntTy::get(Context &c, uint16_t _bits, bool _sign)
{
	static Map<uint32_t, IntTy *> resmap;
	auto loc = resmap.find(_bits + (_sign * 2));
	if(loc != resmap.end()) return loc->second;
	IntTy *res		    = c.allocType<IntTy>(_bits, _sign);
	resmap[_bits + (_sign * 2)] = res;
	return res;
}

Value *IntTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd, size_t weak_depth)
{
	return IntVal::create(c, cd, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Float Type
///////////////////////////////////////////////////////////////////////////////////////////////////

FltTy::FltTy(uint16_t bits) : Type(TFLT), bits(bits) {}
FltTy::~FltTy() {}

Type *FltTy::specialize(Context &c, size_t weak_depth) { return this; }
uint32_t FltTy::getID()
{
	return getBaseID() + (bits * 3); // * 3 to prevent clash between int and flt
}
String FltTy::toStr(size_t weak_depth) { return "f" + std::to_string(bits); }

FltTy *FltTy::get(Context &c, uint16_t _bits)
{
	static Map<uint32_t, FltTy *> resmap;
	auto loc = resmap.find(_bits);
	if(loc != resmap.end()) return loc->second;
	FltTy *res    = c.allocType<FltTy>(_bits);
	resmap[_bits] = res;
	return res;
}

Value *FltTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd, size_t weak_depth)
{
	return FltVal::create(c, cd, 0.0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Type Type
///////////////////////////////////////////////////////////////////////////////////////////////////

TypeTy::TypeTy() : Type(TTYPE), containedtyid(genContainedTypeID()) {}
TypeTy::TypeTy(uint32_t containedtyid) : Type(TTYPE), containedtyid(containedtyid) {}
TypeTy::~TypeTy() {}

Type *TypeTy::specialize(Context &c, size_t weak_depth)
{
	if(getContainedTy()) {
		return getContainedTy()->specialize(c, weak_depth);
	}
	return this;
}
uint32_t TypeTy::getUniqID()
{
	if(getContainedTy()) return getContainedTy()->getUniqID();
	return getID();
}
uint32_t TypeTy::getID() { return getBaseID(); }
bool TypeTy::isTemplate(size_t weak_depth) { return !getContainedTy(); }
String TypeTy::toStr(size_t weak_depth)
{
	Type *ct = getContainedTy();
	return "typety<" +
	       (ct ? ct->toStr(weak_depth) : "(none:" + std::to_string(containedtyid) + ")") + ">";
}
bool TypeTy::mergeTemplatesFrom(Type *ty, size_t weak_depth)
{
	if(getContainedTy()) return true;
	if(!ty->isTypeTy()) {
		setContainedTy(ty);
		return true;
	}
	if(!as<TypeTy>(ty)->getContainedTy()) return true;
	setContainedTy(as<TypeTy>(ty)->getContainedTy());
	return true;
}
void TypeTy::unmergeTemplates(size_t weak_depth) { clearContainedTy(); }

TypeTy *TypeTy::get(Context &c) { return c.allocType<TypeTy>(); }

void TypeTy::clearContainedTy() { containedtypes[containedtyid] = nullptr; }
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

Value *TypeTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd, size_t weak_depth)
{
	if(!getContainedTy()) {
		return TypeVal::create(c, this);
	}
	return getContainedTy()->toDefaultValue(c, loc, cd, weak_depth);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Pointer Type
///////////////////////////////////////////////////////////////////////////////////////////////////

PtrTy::PtrTy(Type *to, uint64_t count, bool is_weak)
	: Type(TPTR), to(to), count(count), is_weak(is_weak)
{}
PtrTy::~PtrTy() {}

Type *PtrTy::specialize(Context &c, size_t weak_depth)
{
	bool valid_depth = weak_depth < MAX_WEAKPTR_DEPTH;
	Type *res	 = valid_depth ? to->specialize(c, weak_depth + is_weak) : to;
	if(res == to) return this;
	return c.allocType<PtrTy>(res, count, is_weak);
}
uint32_t PtrTy::getUniqID()
{
	if(to && !is_weak) return to->getUniqID() + getID();
	return getID();
}
uint32_t PtrTy::getID() { return getBaseID() + count * 17; }
bool PtrTy::isTemplate(size_t weak_depth)
{
	return weak_depth >= MAX_WEAKPTR_DEPTH ? false : to->isTemplate(weak_depth + is_weak);
}
String PtrTy::toStr(size_t weak_depth)
{
	String extradata;
	if(count) extradata = "[" + std::to_string(count) + "] ";
	String res = "*" + extradata;
	if(weak_depth) {
		res += " weak<" + std::to_string(to->getID()) + ">";
	} else {
		res += to->toStr(weak_depth + is_weak);
	}
	return res;
}
bool PtrTy::mergeTemplatesFrom(Type *ty, size_t weak_depth)
{
	if(weak_depth >= MAX_WEAKPTR_DEPTH) return false;
	if(!ty->isPtr()) return false;
	return to->mergeTemplatesFrom(as<PtrTy>(ty)->getTo(), weak_depth + is_weak);
}
void PtrTy::unmergeTemplates(size_t weak_depth)
{
	if(weak_depth >= MAX_WEAKPTR_DEPTH) return;
	to->unmergeTemplates(weak_depth + is_weak);
}
PtrTy *PtrTy::get(Context &c, Type *ptr_to, uint64_t count, bool is_weak)
{
	return c.allocType<PtrTy>(ptr_to, count, is_weak);
}
PtrTy *PtrTy::getStr(Context &c)
{
	static PtrTy *res = PtrTy::get(c, IntTy::get(c, 8, true), 0, false);
	return res;
}
PtrTy *PtrTy::getStr(Context &c, size_t count)
{
	Type *ch   = IntTy::get(c, 8, true);
	PtrTy *res = PtrTy::get(c, ch, count, false);
	return res;
}

Value *PtrTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd, size_t weak_depth)
{
	if(!count) return IntVal::create(c, cd, 0);
	Vector<Value *> vec;
	vec.reserve(count);
	Value *res = weak_depth >= MAX_WEAKPTR_DEPTH
		     ? IntVal::create(c, cd, 0)
		     : to->toDefaultValue(c, loc, cd, weak_depth + is_weak);
	if(!res) {
		err::out(loc, "failed to get default value from array's type");
		return nullptr;
	}
	vec.push_back(res);
	for(size_t i = 1; i < count; ++i) {
		vec.push_back(res->clone(c));
	}
	return VecVal::create(c, cd, vec);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Struct Type
///////////////////////////////////////////////////////////////////////////////////////////////////

StructTy::StructTy(StmtStruct *decl, const Vector<StringRef> &fieldnames,
		   const Vector<Type *> &fields, const Vector<StringRef> &templatenames,
		   const Vector<TypeTy *> &templates, bool externed)
	: Type(TSTRUCT), id(genTypeID()), decl(decl), fieldnames(fieldnames), fields(fields),
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
StructTy::StructTy(uint32_t id, StmtStruct *decl, const Vector<StringRef> &fieldnames,
		   const Map<StringRef, size_t> &fieldpos, const Vector<Type *> &fields,
		   const Vector<StringRef> &templatenames,
		   const Map<StringRef, size_t> &templatepos, const Vector<TypeTy *> &templates,
		   bool has_template, bool externed)
	: Type(TSTRUCT), id(id), decl(decl), fieldpos(fieldpos), fieldnames(fieldnames),
	  fields(fields), templatepos(templatepos), templatenames(templatenames),
	  templates(templates), has_template(has_template), externed(externed)
{}
StructTy::~StructTy() {}

Type *StructTy::specialize(Context &c, size_t weak_depth)
{
	if(templates.empty()) return this;
	Vector<Type *> newfields;
	newfields.reserve(fields.size());
	Vector<TypeTy *> newtemplates;
	newtemplates.reserve(templates.size());
	for(auto &field : fields) {
		newfields.push_back(field->specialize(c, weak_depth));
	}
	for(auto &t : templates) {
		newtemplates.push_back(as<TypeTy>(t->specialize(c, weak_depth)));
	}
	return c.allocType<StructTy>(id, decl, fieldnames, fieldpos, newfields, templatenames,
				     templatepos, newtemplates, has_template, externed);
}
uint32_t StructTy::getUniqID()
{
	uint32_t res = 0;
	for(auto &f : fields) {
		res += f->getUniqID();
	}
	uint32_t tmpres	    = res;
	uint32_t multiplier = 1;
	while(tmpres) {
		multiplier *= 10;
		tmpres /= 10;
	}
	return getID() * multiplier + res;
}
uint32_t StructTy::getID() { return id; }
bool StructTy::isTemplate(size_t weak_depth)
{
	for(auto &f : fields) {
		if(f->isTemplate(weak_depth)) return true;
	}
	return false;
}
String StructTy::toStr(size_t weak_depth)
{
	String res = "struct<" + std::to_string(getID()) + ">{";
	for(auto &f : fields) {
		res += f->toStr(weak_depth) + ", ";
	}
	if(fields.size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += "}";
	return res;
}
bool StructTy::mergeTemplatesFrom(Type *ty, size_t weak_depth)
{
	if(!ty->isStruct()) return false;
	StructTy *other = as<StructTy>(ty);
	if(fields.size() != other->fields.size()) return false;
	bool has_templ = false;
	for(size_t i = 0; i < fields.size(); ++i) {
		has_templ |= fields[i]->mergeTemplatesFrom(other->fields[i], weak_depth);
	}
	return has_templ;
}
void StructTy::unmergeTemplates(size_t weak_depth)
{
	for(auto &f : fields) f->unmergeTemplates(weak_depth);
}
bool StructTy::isCompatible(Context &c, Type *rhs, const ModuleLoc *loc)
{
	if(!isBaseCompatible(c, rhs, loc)) return false;
	StructTy *r = as<StructTy>(rhs);
	if(fields.size() != r->fields.size()) {
		err::out(loc, "struct type mismatch (LHS fields: ", fields.size(),
			 ", RHS fields: ", r->fields.size(), ")");
		return false;
	}
	for(size_t i = 0; i < fields.size(); ++i) {
		if(fields[i]->isCompatible(c, r->fields[i], loc)) continue;
		err::out(loc, "LHS struct field ", fields[i]->toStr(), " with index ", i,
			 ", incompatible with RHS field ", r->fields[i]->toStr());
		return false;
	}
	return true;
}
StructTy *StructTy::applyTemplates(Context &c, const ModuleLoc *loc, const Vector<Type *> &actuals)
{
	if(templates.size() != actuals.size()) {
		err::out(loc, "expected templates for struct: ", templates.size(),
			 ", found: ", actuals.size());
		return nullptr;
	}
	for(size_t i = 0; i < templates.size(); ++i) {
		templates[i]->setContainedTy(actuals[i]);
	}
	StructTy *res = as<StructTy>(this->specialize(c));
	for(auto &t : templates) {
		t->clearContainedTy();
	}
	res->setTemplate(false);
	return res;
}
StructTy *StructTy::instantiate(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &callargs)
{
	if(fields.size() != callargs.size()) return nullptr;
	if(isTemplate()) {
		err::out(loc, "a struct with templates cannot be instantiated");
		return nullptr;
	}
	bool is_field_compatible = true;
	for(size_t i = 0; i < this->fields.size(); ++i) {
		Type *sf    = this->fields[i];
		Stmt *ciarg = callargs[i];
		if(sf->isCompatible(c, ciarg->getTy(), loc)) continue;
		is_field_compatible = false;
		break;
	}
	if(!is_field_compatible) return nullptr;
	return as<StructTy>(this->specialize(c));
}
StructTy *StructTy::get(Context &c, StmtStruct *decl, const Vector<StringRef> &_fieldnames,
			const Vector<Type *> &_fields, const Vector<StringRef> &_templatenames,
			const Vector<TypeTy *> &_templates, bool _externed)
{
	return c.allocType<StructTy>(decl, _fieldnames, _fields, _templatenames, _templates,
				     _externed);
}
void StructTy::setStrRef(StructTy *ty)
{
	if(strrefty) ty->id = strrefty->id;
	strrefty = ty;
}
StructTy *StructTy::getStrRef(Context &c)
{
	if(!strrefty) {
		Type *s	 = PtrTy::getStr(c);
		Type *i	 = IntTy::get(c, 64, false);
		strrefty = StructTy::get(c, nullptr, {"data", "length"}, {s, i}, {}, {}, false);
	}
	return strrefty;
}
Type *StructTy::getField(StringRef name)
{
	auto templfound = templatepos.find(name);
	if(templfound != templatepos.end()) return templates[templfound->second];
	auto found = fieldpos.find(name);
	if(found == fieldpos.end()) return nullptr;
	return fields[found->second];
}
Type *StructTy::getField(size_t pos)
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

Value *StructTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
				size_t weak_depth)
{
	Map<StringRef, Value *> st;
	for(auto &f : fieldpos) {
		Value *res = fields[f.second]->toDefaultValue(c, loc, cd, weak_depth);
		if(!res) {
			err::out(loc, "failed to get default value from struct field type");
			return nullptr;
		}
		st[f.first] = res;
	}
	for(auto &t : templatepos) {
		st[t.first] = TypeVal::create(c, templates[t.second]);
	}
	return StructVal::create(c, cd, st);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Function Type
///////////////////////////////////////////////////////////////////////////////////////////////////

FuncTy::FuncTy(StmtVar *var, const Vector<Type *> &args, Type *ret,
	       const Vector<bool> &_argcomptime, IntrinsicFn intrin, IntrinType inty, bool externed,
	       bool variadic)
	: Type(TFUNC), id(genTypeID()), var(var), sig(nullptr), args(args), ret(ret),
	  argcomptime(_argcomptime), intrin(intrin), inty(inty),
	  uniqid(!externed ? genFuncUniqID() : 0), externed(externed), variadic(variadic)
{
	setSigFromVar();
	if(_argcomptime.empty() && !args.empty()) {
		argcomptime = Vector<bool>(args.size(), false);
	}
}
FuncTy::FuncTy(uint32_t id, StmtVar *var, StmtFnSig *sig, const Vector<Type *> &args, Type *ret,
	       const Vector<bool> &_argcomptime, IntrinsicFn intrin, IntrinType inty,
	       uint32_t uniqid, bool externed, bool variadic)
	: Type(TFUNC), id(id), var(var), sig(sig), args(args), ret(ret), argcomptime(_argcomptime),
	  intrin(intrin), inty(inty), uniqid(uniqid), externed(externed), variadic(variadic)
{
	if(_argcomptime.empty() && !args.empty()) {
		argcomptime = Vector<bool>(args.size(), false);
	}
}
FuncTy::~FuncTy() {}

Type *FuncTy::specialize(Context &c, size_t weak_depth)
{
	Vector<Type *> newargs;
	newargs.reserve(args.size());
	for(auto &arg : args) newargs.push_back(arg->specialize(c, weak_depth));
	return c.allocType<FuncTy>(id, var, sig, newargs, ret->specialize(c, weak_depth),
				   argcomptime, intrin, inty, uniqid, externed, variadic);
}
uint32_t FuncTy::getSignatureID()
{
	uint32_t res = TFUNC;
	for(auto &a : args) {
		res += a->getUniqID();
	}
	res += ret->getUniqID();
	return res * 7;
}
uint32_t FuncTy::getUniqID()
{
	uint32_t res   = uniqid;
	uint32_t tmpid = id;
	while(tmpid) {
		res *= 10;
		tmpid /= 10;
	}
	res += id;
	for(auto &a : args) {
		res += a->getUniqID();
	}
	res += ret->getUniqID();
	return res * 7;
}
uint32_t FuncTy::getID()
{
	uint32_t res   = uniqid;
	uint32_t tmpid = id;
	while(tmpid) {
		res *= 10;
		tmpid /= 10;
	}
	res += id;
	for(auto &a : args) {
		res += a->getID();
	}
	res += ret->getID();
	return res;
}
bool FuncTy::isTemplate(size_t weak_depth)
{
	for(auto &a : args) {
		if(a->isTemplate(weak_depth)) return true;
	}
	return ret->isTemplate(weak_depth);
}
String FuncTy::toStr(size_t weak_depth)
{
	String res = "function<" + std::to_string(getID());
	if(intrin || externed) res += ", ";
	if(intrin) res += "intrinsic, ";
	if(externed) res += "extern, ";
	if(intrin || externed) {
		res.pop_back();
		res.pop_back();
	}
	res += ">(";
	for(auto &a : args) {
		res += a->toStr(weak_depth) + ", ";
	}
	if(args.size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += "): " + ret->toStr(weak_depth);
	return res;
}
bool FuncTy::mergeTemplatesFrom(Type *ty, size_t weak_depth)
{
	if(!ty->isFunc()) return false;
	FuncTy *other = as<FuncTy>(ty);
	if(args.size() != other->args.size()) return false;
	bool has_templ = false;
	for(size_t i = 0; i < args.size(); ++i) {
		has_templ |= args[i]->mergeTemplatesFrom(other->args[i], weak_depth);
	}
	has_templ |= ret->mergeTemplatesFrom(other->ret, weak_depth);
	return has_templ;
}
void FuncTy::unmergeTemplates(size_t weak_depth)
{
	for(auto &a : args) a->unmergeTemplates(weak_depth);
	ret->unmergeTemplates(weak_depth);
}
bool FuncTy::isCompatible(Context &c, Type *rhs, const ModuleLoc *loc)
{
	if(!isBaseCompatible(c, rhs, loc)) return false;
	FuncTy *r = as<FuncTy>(rhs);
	if(externed != r->externed) {
		err::out(loc, "func type mismatch (LHS externed: ", externed ? "yes" : "no",
			 ", RHS externed: ", r->externed ? "yes" : "no", ")");
	}
	if(args.size() != r->args.size()) {
		err::out(loc, "type mismatch (LHS args: ", args.size(),
			 ", RHS args: ", r->args.size());
		return false;
	}
	for(size_t i = 0; i < args.size(); ++i) {
		if(args[i]->isCompatible(c, r->args[i], loc)) continue;
		err::out(loc, "LHS function arg ", args[i]->toStr(), " with index ", i,
			 ", incompatible with RHS arg ", r->args[i]->toStr());
		return false;
	}
	if(!ret->isCompatible(c, r->ret, loc)) {
		err::out(loc, "incompatible return types (LHS: ", ret->toStr(),
			 ", RHS: ", r->ret->toStr(), ")");
		return false;
	}
	return true;
}
// specializes a function type using StmtFnCallInfo
FuncTy *FuncTy::createCall(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &callargs)
{
	if(args.size() - isVariadic() > callargs.size()) return nullptr;
	if(args.size() != callargs.size() && !isVariadic()) return nullptr;
	bool is_arg_compatible = true;
	Vector<Type *> variadics;
	bool has_templ = false;
	for(size_t i = 0; i < args.size() && i < callargs.size(); ++i) {
		has_templ |= args[i]->mergeTemplatesFrom(callargs[i]->getTy());
	}
	for(size_t i = 0, j = 0; i < this->args.size() && j < callargs.size(); ++i, ++j) {
		Type *sa      = this->args[i];
		StmtVar *sigv = getSig() ? getSig()->getArg(i) : nullptr;
		Stmt *ciarg   = callargs[j];
		bool variadic = false;
		if(sig && ciarg->isConst() && !sig->getArg(i)->isConst() &&
		   (isPtr() || sig->getArg(i)->isRef()))
		{
			err::out(loc, "losing constness for argument index ", std::to_string(i),
				 ", cannot continue");
			is_arg_compatible = false;
			break;
		}
		if(i == this->args.size() - 1 && isVariadic()) {
			variadic = true;
			--i;
		}
		if(!sa->isCompatible(c, ciarg->getTy(), loc)) {
			is_arg_compatible = false;
			break;
		}
		if(sigv && ciarg->isSimple() &&
		   as<StmtSimple>(ciarg)->getLexValue().getTokVal() != lex::IDEN &&
		   !sigv->isConst() && sigv->isRef())
		{
			err::out(loc, "cannot pass literal data to a "
				      "function expecting a non-const reference");
			is_arg_compatible = false;
			break;
		}
		if(variadic) variadics.push_back(ciarg->getTy());
	}
	if(!is_arg_compatible) return nullptr;

	FuncTy *res = this;
	if(isVariadic()) {
		res	     = as<FuncTy>(specialize(c));
		Type *vabase = res->args.back();
		res->args.pop_back();
		VariadicTy *va	= VariadicTy::get(c, {});
		size_t ptrcount = getPointerCount(vabase);
		for(auto &vtmp : variadics) {
			Type *v = vtmp->specialize(c);
			applyPointerCount(c, v, ptrcount);
			va->addArg(v);
		}
		res->args.push_back(va);
		has_templ = true;
	}
	res = as<FuncTy>(res->specialize(c));
	if(has_templ) {
		unmergeTemplates();
		res->updateUniqID();
	}
	for(size_t i = 0; i < res->args.size(); ++i) {
		if(res->args[i]->isAny()) {
			res->args[i] = callargs[i]->getTy()->specialize(c);
		}
	}
	return res;
}
FuncTy *FuncTy::get(Context &c, StmtVar *_var, const Vector<Type *> &_args, Type *_ret,
		    const Vector<bool> &_argcomptime, IntrinsicFn _intrin, IntrinType _inty,
		    bool _externed, bool _variadic)
{
	return c.allocType<FuncTy>(_var, _args, _ret, _argcomptime, _intrin, _inty, _externed,
				   _variadic);
}
void FuncTy::setSigFromVar()
{
	if(!var) return;
	if(var->getVVal()->isFnDef()) {
		sig = as<StmtFnDef>(var->getVVal())->getSig();
	} else if(var->getVVal()->isExtern()) {
		sig = as<StmtFnSig>(as<StmtExtern>(var->getVVal())->getEntity());
	}
}
void FuncTy::updateUniqID() { uniqid = genFuncUniqID(); }
bool FuncTy::callIntrinsic(Context &c, StmtExpr *stmt, Stmt **source, Vector<Stmt *> &callargs)
{
	return intrin(c, stmt, source, callargs);
}
Value *FuncTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd, size_t weak_depth)
{
	return FuncVal::create(c, this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Variadic Type
///////////////////////////////////////////////////////////////////////////////////////////////////

VariadicTy::VariadicTy(const Vector<Type *> &args) : Type(TVARIADIC), args(args) {}
VariadicTy::~VariadicTy() {}

Type *VariadicTy::specialize(Context &c, size_t weak_depth)
{
	Vector<Type *> newargs;
	newargs.reserve(args.size());
	for(auto &arg : args) newargs.push_back(arg->specialize(c, weak_depth));
	return c.allocType<VariadicTy>(newargs);
}
bool VariadicTy::isTemplate(size_t weak_depth)
{
	for(auto &a : args) {
		if(a->isTemplate(weak_depth)) return true;
	}
	return false;
}
String VariadicTy::toStr(size_t weak_depth)
{
	String res = "variadic<";
	for(auto &a : args) {
		res += a->toStr(weak_depth) + ", ";
	}
	if(args.size() > 0) {
		res.pop_back();
		res.pop_back();
	}
	res += ">";
	return res;
}
bool VariadicTy::mergeTemplatesFrom(Type *ty, size_t weak_depth)
{
	if(!ty->isVariadic()) return false;
	VariadicTy *other = as<VariadicTy>(ty);
	if(args.size() != other->args.size()) return false;
	bool has_templ = false;
	for(size_t i = 0; i < args.size(); ++i) {
		has_templ |= args[i]->mergeTemplatesFrom(other->args[i], weak_depth);
	}
	return has_templ;
}
void VariadicTy::unmergeTemplates(size_t weak_depth)
{
	for(auto &a : args) a->unmergeTemplates(weak_depth);
}
bool VariadicTy::isCompatible(Context &c, Type *rhs, const ModuleLoc *loc)
{
	if(!isBaseCompatible(c, rhs, loc)) return false;
	VariadicTy *r = as<VariadicTy>(rhs);
	if(args.size() != r->args.size()) return false;
	for(size_t i = 0; i < args.size(); ++i) {
		if(!args[i]->isCompatible(c, r->args[i], loc)) return false;
	}
	return true;
}
VariadicTy *VariadicTy::get(Context &c, const Vector<Type *> &_args)
{
	return c.allocType<VariadicTy>(_args);
}
Value *VariadicTy::toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
				  size_t weak_depth)
{
	Vector<Value *> vec;
	vec.reserve(args.size());
	for(auto &a : args) {
		Value *v = a->toDefaultValue(c, loc, cd, weak_depth);
		if(!v) {
			err::out(loc, "failed to generate default value for type: ", a->toStr());
			return nullptr;
		}
		vec.push_back(v);
	}
	return VecVal::create(c, cd, vec);
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
Type *applyPointerCount(Context &c, Type *t, uint16_t count)
{
	for(size_t i = 0; i < count; ++i) {
		t = PtrTy::get(c, t, 0, false);
	}
	return t;
}
} // namespace sc
