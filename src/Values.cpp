#include "Values.hpp"

#include "Types.hpp"

namespace sc
{
Value::Value(const Values &vty, ContainsData has_data) : vty(vty), has_data(has_data) {}
Value::~Value() {}
ContainsData Value::getHasData() { return has_data; }
void Value::setHasData(ContainsData cd)
{
	if(has_data == CDPERMA) return;
	has_data = cd;
}
void Value::setContainsData()
{
	if(has_data == CDPERMA) return;
	has_data = CDTRUE;
}
void Value::setContainsPermaData() { has_data = CDPERMA; }
void Value::unsetContainsPermaData()
{
	if(has_data == CDPERMA) has_data = CDTRUE;
}
bool Value::hasData() { return has_data == CDTRUE || has_data == CDPERMA; }
bool Value::hasPermaData() { return has_data == CDPERMA; }
void Value::clearHasData()
{
	if(has_data == CDPERMA) return;
	has_data = CDFALSE;
}

IntVal::IntVal(Context &c, ContainsData has_data, int64_t data) : Value(VINT, has_data), data(data)
{}

String IntVal::toStr() { return std::to_string(data); }
Value *IntVal::clone(Context &c)
{
	return create(c, has_data == CDPERMA ? CDTRUE : has_data, data);
}
bool IntVal::updateValue(Context &c, Value *v)
{
	if(!v->isInt()) return false;
	data	 = as<IntVal>(v)->getVal();
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}

IntVal *IntVal::create(Context &c, ContainsData has_data, int64_t val)
{
	return c.allocVal<IntVal>(has_data, val);
}

FltVal::FltVal(Context &c, ContainsData has_data, const long double &data)
	: Value(VFLT, has_data), data(data)
{}

String FltVal::toStr() { return std::to_string(data); }
Value *FltVal::clone(Context &c) { return create(c, has_data, data); }
bool FltVal::updateValue(Context &c, Value *v)
{
	if(!v->isFlt()) return false;
	data	 = as<FltVal>(v)->getVal();
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}

FltVal *FltVal::create(Context &c, ContainsData has_data, const long double &val)
{
	return c.allocVal<FltVal>(has_data, val);
}

VecVal::VecVal(Context &c, ContainsData has_data, const Vector<Value *> &data)
	: Value(VVEC, has_data), data(data)
{}

String VecVal::toStr()
{
	String res = "[";
	for(size_t i = 0; i < data.size(); ++i) {
		res += data[i]->toStr();
		if(i < data.size() - 1) res += ", ";
	}
	res += "]";
	return res;
}
Value *VecVal::clone(Context &c)
{
	Vector<Value *> newdata;
	for(auto &d : data) {
		newdata.push_back(d->clone(c));
	}
	return create(c, has_data == CDPERMA ? CDTRUE : has_data, newdata);
}
bool VecVal::updateValue(Context &c, Value *v)
{
	if(!v->isVec()) return false;
	VecVal *vv = as<VecVal>(v);
	if(data.size() != vv->getVal().size()) return false;
	for(size_t i = 0; i < data.size(); ++i) {
		if(!data[i]->updateValue(c, vv->getValAt(i))) return false;
	}
end:
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}

VecVal *VecVal::create(Context &c, ContainsData has_data, const Vector<Value *> &val)
{
	return c.allocVal<VecVal>(has_data, val);
}
VecVal *VecVal::createStr(Context &c, ContainsData has_data, StringRef val)
{
	Vector<Value *> chars;
	for(auto &ch : val) {
		chars.push_back(IntVal::create(c, has_data, ch));
	}
	return create(c, has_data, chars);
}
String VecVal::getAsString()
{
	String res;
	for(auto &ch : data) {
		res.push_back(as<IntVal>(ch)->getVal());
	}
	return res;
}

StructVal::StructVal(Context &c, ContainsData has_data, const Map<StringRef, Value *> &data)
	: Value(VSTRUCT, has_data), data(data)
{}

String StructVal::toStr()
{
	String res = "{";
	for(auto &d : data) {
		res += String(d.first) + ": " + d.second->toStr() + ", ";
	}
	if(!data.empty()) {
		res.pop_back();
		res.pop_back();
	}
	res += "}";
	return res;
}
Value *StructVal::clone(Context &c)
{
	Map<StringRef, Value *> newdata;
	for(auto &d : data) {
		newdata[d.first] = d.second->clone(c);
	}
	return create(c, has_data == CDPERMA ? CDTRUE : has_data, newdata);
}
bool StructVal::updateValue(Context &c, Value *v)
{
	if(!v->isStruct()) return false;
	StructVal *sv = as<StructVal>(v);
	if(data.size() != sv->getVal().size()) return false;
	for(auto &f : data) {
		if(!f.second->updateValue(c, sv->getField(f.first))) return false;
	}
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}
StructVal *StructVal::create(Context &c, ContainsData has_data, const Map<StringRef, Value *> &val)
{
	return c.allocVal<StructVal>(has_data, val);
}
StructVal *StructVal::createStrRef(Context &c, ContainsData has_data, StringRef val)
{
	Value *s		  = VecVal::createStr(c, has_data, val);
	Value *i		  = IntVal::create(c, has_data, val.size());
	Map<StringRef, Value *> m = {{"data", s}, {"length", i}};
	return create(c, has_data, m);
}
String StructVal::getStrFromRef() { return as<VecVal>(getField("data"))->getAsString(); }

FuncVal::FuncVal(Context &c, FuncTy *val) : Value(VFUNC, CDPERMA), ty(val) {}

String FuncVal::toStr() { return "func<" + ty->toStr() + ">"; }
Value *FuncVal::clone(Context &c) { return create(c, as<FuncTy>(ty)); }
bool FuncVal::updateValue(Context &c, Value *v) { return true; }

FuncVal *FuncVal::create(Context &c, FuncTy *val) { return c.allocVal<FuncVal>(val); }

TypeVal::TypeVal(Context &c, Type *val) : Value(VTYPE, CDPERMA), ty(val) {}

String TypeVal::toStr() { return "typeval<" + ty->toStr() + ">"; }
Value *TypeVal::clone(Context &c) { return create(c, ty); }
bool TypeVal::updateValue(Context &c, Value *v) { return true; }

TypeVal *TypeVal::create(Context &c, Type *val) { return c.allocVal<TypeVal>(val); }

NamespaceVal::NamespaceVal(Context &c, StringRef val) : Value(VNAMESPACE, CDPERMA), val(val) {}

String NamespaceVal::toStr() { return String(val); }
Value *NamespaceVal::clone(Context &c) { return create(c, val); }
bool NamespaceVal::updateValue(Context &c, Value *v)
{
	if(!v->isNamespace()) return false;
	val	 = as<NamespaceVal>(v)->getVal();
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}

NamespaceVal *NamespaceVal::create(Context &c, StringRef val)
{
	return c.allocVal<NamespaceVal>(val);
}
} // namespace sc
