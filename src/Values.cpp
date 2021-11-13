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

#include "Values.hpp"

namespace sc
{
Value::Value(const Values &vty, Type *ty, bool has_val) : vty(vty), ty(ty), has_val(has_val) {}
Value::~Value() {}
bool Value::IsValStrLiteral()
{
	if(!ty->isPtr() || as<PtrTy>(ty)->getCount()) return false;
	Type *inner = as<PtrTy>(ty)->getTo();
	if(!inner->isInt()) return false;
	IntTy *i = as<IntTy>(inner);
	if(i->getBits() != 8 || !i->isSigned()) return false;
	return true;
}
void Value::setHasData(const bool &val)
{
	has_val = val;
}
bool Value::hasData()
{
	return has_val;
}

VoidVal::VoidVal(Context &c) : Value(VVOID, VoidTy::create(c), true) {}

std::string VoidVal::toStr()
{
	return "<void>";
}
Value *VoidVal::clone(Context &c)
{
	return create(c);
}
bool VoidVal::updateValue(Value *v)
{
	return v->isVoid();
}

VoidVal *VoidVal::create(Context &c)
{
	return c.allocVal<VoidVal>();
}

IntVal::IntVal(Context &c, Type *ty, bool has_val, const int64_t &data)
	: Value(VINT, ty, has_val), data(data)
{}

std::string IntVal::toStr()
{
	return std::to_string(data);
}
Value *IntVal::clone(Context &c)
{
	return create(c, ty, has_val, data);
}
bool IntVal::updateValue(Value *v)
{
	if(!v->isInt()) return false;
	data	= as<IntVal>(v)->getVal();
	has_val = v->hasData();
	return true;
}

IntVal *IntVal::create(Context &c, Type *ty, bool has_val, const int64_t &val)
{
	return c.allocVal<IntVal>(ty, has_val, val);
}

FltVal::FltVal(Context &c, Type *ty, bool has_val, const long double &data)
	: Value(VFLT, ty, has_val), data(data)
{}

std::string FltVal::toStr()
{
	return std::to_string(data);
}
Value *FltVal::clone(Context &c)
{
	return create(c, ty, has_val, data);
}
bool FltVal::updateValue(Value *v)
{
	if(!v->isFlt()) return false;
	data	= as<FltVal>(v)->getVal();
	has_val = v->hasData();
	return true;
}

FltVal *FltVal::create(Context &c, Type *ty, bool has_val, const long double &val)
{
	return c.allocVal<FltVal>(ty, has_val, val);
}

VecVal::VecVal(Context &c, Type *ty, bool has_val, const std::vector<Value *> &data)
	: Value(VVEC, ty, has_val), data(data)
{}

std::string VecVal::toStr()
{
	std::string res = "[";
	for(size_t i = 0; i < data.size(); ++i) {
		res += data[i]->toStr();
		if(i < data.size() - 1) res += ", ";
	}
	res += "]";
	return res;
}
Value *VecVal::clone(Context &c)
{
	std::vector<Value *> newdata;
	for(auto &d : data) {
		newdata.push_back(d->clone(c));
	}
	return create(c, ty, has_val, newdata);
}
bool VecVal::updateValue(Value *v)
{
	if(!v->isVec()) return false;
	VecVal *vv = as<VecVal>(v);
	if(data.size() != vv->getVal().size()) return false;
	for(size_t i = 0; i < data.size(); ++i) {
		if(!data[i]->updateValue(vv->getValAt(i))) return false;
	}
	has_val = v->hasData();
	return true;
}

VecVal *VecVal::create(Context &c, Type *ty, bool has_val, const std::vector<Value *> &val)
{
	return c.allocVal<VecVal>(ty, has_val, val);
}
VecVal *VecVal::createStr(Context &c, const std::string &val)
{
	std::vector<Value *> chars;
	Type *ty = mkI8Ty(c);
	for(auto &ch : val) {
		chars.push_back(IntVal::create(c, ty, true, ch));
	}
	ty = mkPtrTy(c, ty, 0);
	return c.allocVal<VecVal>(ty, true, chars);
}

std::string VecVal::getAsString()
{
	std::string res;
	for(auto &ch : data) {
		if(!ch->getType()->isInt()) return "";
		if(as<IntTy>(ch->getType())->getBits() != 8) return "";
		if(!as<IntTy>(ch->getType())->isSigned()) return "";
		res.push_back(as<IntVal>(ch)->getVal());
	}
	return res;
}

StructVal::StructVal(Context &c, Type *ty, bool has_val,
		     const std::unordered_map<std::string, Value *> &data)
	: Value(VSTRUCT, ty, has_val), data(data)
{}

std::string StructVal::toStr()
{
	std::string res = "{";
	for(auto &d : data) {
		res += d.first + ": " + d.second->toStr() + ", ";
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
	std::unordered_map<std::string, Value *> newdata;
	for(auto &d : data) {
		newdata[d.first] = d.second->clone(c);
	}
	return create(c, ty, has_val, newdata);
}
bool StructVal::updateValue(Value *v)
{
	if(!v->isStruct()) return false;
	StructVal *sv = as<StructVal>(v);
	if(data.size() != sv->getVal().size()) return false;
	for(auto &f : data) {
		if(!f.second->updateValue(sv->getField(f.first))) return false;
	}
	has_val = v->hasData();
	return true;
}

StructVal *StructVal::create(Context &c, Type *ty, bool has_val,
			     const std::unordered_map<std::string, Value *> &val)
{
	return c.allocVal<StructVal>(ty, has_val, val);
}

FuncVal::FuncVal(Context &c, FuncTy *val) : Value(VFUNC, val, true) {}

std::string FuncVal::toStr()
{
	return "func<" + ty->toStr() + ">";
}
Value *FuncVal::clone(Context &c)
{
	return create(c, as<FuncTy>(ty->clone(c)));
}
bool FuncVal::updateValue(Value *v)
{
	return true;
}

FuncVal *FuncVal::create(Context &c, FuncTy *val)
{
	return c.allocVal<FuncVal>(val);
}

TypeVal::TypeVal(Context &c, Type *val) : Value(VTYPE, val, true) {}

std::string TypeVal::toStr()
{
	return "typeval<" + ty->toStr() + ">";
}
Value *TypeVal::clone(Context &c)
{
	return create(c, ty->clone(c));
}
bool TypeVal::updateValue(Value *v)
{
	return true;
}

TypeVal *TypeVal::create(Context &c, Type *val)
{
	return c.allocVal<TypeVal>(val);
}

ImportVal::ImportVal(Context &c, const std::string &val)
	: Value(VIMPORT, mkStrTy(c), true), val(val)
{}

std::string ImportVal::toStr()
{
	return val;
}
Value *ImportVal::clone(Context &c)
{
	return create(c, val);
}
bool ImportVal::updateValue(Value *v)
{
	if(!v->isImport()) return false;
	val	= as<ImportVal>(v)->getVal();
	has_val = v->hasData();
	return true;
}

ImportVal *ImportVal::create(Context &c, const std::string &val)
{
	return c.allocVal<ImportVal>(val);
}

RefVal::RefVal(Context &c, Type *ty, Value *to) : Value(VREF, ty, true), to(to) {}

std::string RefVal::toStr()
{
	return "ref<" + to->toStr() + ">";
}
Value *RefVal::clone(Context &c)
{
	return create(c, ty, to->clone(c));
}
bool RefVal::updateValue(Value *v)
{
	if(!v->isRef()) return false;
	to->setHasData(v->hasData());
	return to->updateValue(as<RefVal>(v)->getVal());
}

RefVal *RefVal::create(Context &c, Type *ty, Value *to)
{
	return c.allocVal<RefVal>(ty, to);
}
void RefVal::setHasData(const bool &val)
{
	to->setHasData(val);
}
bool RefVal::hasData()
{
	return to->hasData();
}
} // namespace sc
