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

#include "Types.hpp"

namespace sc
{
Value::Value(const Values &vty, Type *ty, ContainsData has_data)
	: vty(vty), ty(ty), has_data(has_data)
{}
Value::~Value() {}
bool Value::isStrLiteral()
{
	if(!ty->isPtr() || as<PtrTy>(ty)->getCount()) return false;
	Type *inner = as<PtrTy>(ty)->getTo();
	if(!inner->isInt()) return false;
	IntTy *i = as<IntTy>(inner);
	if(i->getBits() != 8 || !i->isSigned()) return false;
	return true;
}
ContainsData Value::getHasData()
{
	return has_data;
}
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
void Value::setContainsPermaData()
{
	has_data = CDPERMA;
}
void Value::unsetContainsPermaData()
{
	if(has_data == CDPERMA) has_data = CDTRUE;
}
bool Value::hasData()
{
	return has_data == CDTRUE || has_data == CDPERMA;
}
bool Value::hasPermaData()
{
	return has_data == CDPERMA;
}
void Value::clearHasData()
{
	if(has_data == CDPERMA) return;
	has_data = CDFALSE;
}

VoidVal::VoidVal(Context &c) : Value(VVOID, VoidTy::create(c), CDTRUE) {}

std::string VoidVal::toStr()
{
	return "<void>";
}
Value *VoidVal::clone(Context &c)
{
	return create(c);
}
bool VoidVal::updateValue(Context &c, Value *v)
{
	return v->isVoid();
}

VoidVal *VoidVal::create(Context &c)
{
	return c.allocVal<VoidVal>();
}

IntVal::IntVal(Context &c, Type *ty, ContainsData has_data, const int64_t &data)
	: Value(VINT, ty, has_data), data(data)
{}

std::string IntVal::toStr()
{
	return std::to_string(data);
}
Value *IntVal::clone(Context &c)
{
	return create(c, ty, has_data == CDPERMA ? CDTRUE : has_data, data);
}
bool IntVal::updateValue(Context &c, Value *v)
{
	if(!v->isInt()) return false;
	data	 = as<IntVal>(v)->getVal();
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}

IntVal *IntVal::create(Context &c, Type *ty, ContainsData has_data, const int64_t &val)
{
	return c.allocVal<IntVal>(ty, has_data, val);
}

FltVal::FltVal(Context &c, Type *ty, ContainsData has_data, const long double &data)
	: Value(VFLT, ty, has_data), data(data)
{}

std::string FltVal::toStr()
{
	return std::to_string(data);
}
Value *FltVal::clone(Context &c)
{
	return create(c, ty, has_data == CDPERMA ? CDTRUE : has_data, data);
}
bool FltVal::updateValue(Context &c, Value *v)
{
	if(!v->isFlt()) return false;
	data	 = as<FltVal>(v)->getVal();
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}

FltVal *FltVal::create(Context &c, Type *ty, ContainsData has_data, const long double &val)
{
	return c.allocVal<FltVal>(ty, has_data, val);
}

VecVal::VecVal(Context &c, Type *ty, ContainsData has_data, const std::vector<Value *> &data)
	: Value(VVEC, ty, has_data), data(data)
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
	return create(c, ty, has_data == CDPERMA ? CDTRUE : has_data, newdata);
}
bool VecVal::updateValue(Context &c, Value *v)
{
	if(!v->isVec()) return false;
	VecVal *vv = as<VecVal>(v);
	if(ty->isPtr() && !as<PtrTy>(ty)->getCount()) {
		data.clear();
		// only valid for pointers of unknown size
		for(auto &d : vv->getVal()) data.push_back(d->clone(c));
		goto end;
	}
	if(data.size() != vv->getVal().size()) return false;
	for(size_t i = 0; i < data.size(); ++i) {
		if(!data[i]->updateValue(c, vv->getValAt(i))) return false;
	}
end:
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}

VecVal *VecVal::create(Context &c, Type *ty, ContainsData has_data, const std::vector<Value *> &val)
{
	return c.allocVal<VecVal>(ty, has_data, val);
}
VecVal *VecVal::createStr(Context &c, const std::string &val, ContainsData has_data)
{
	std::vector<Value *> chars;
	Type *ty = mkI8Ty(c);
	for(auto &ch : val) {
		chars.push_back(IntVal::create(c, ty, has_data, ch));
	}
	ty = mkPtrTy(c, ty, 0, false);
	ty->setConst();
	return c.allocVal<VecVal>(ty, has_data, chars);
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

StructVal::StructVal(Context &c, Type *ty, ContainsData has_data,
		     const std::unordered_map<std::string, Value *> &data)
	: Value(VSTRUCT, ty, has_data), data(data)
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
	return create(c, ty, has_data == CDPERMA ? CDTRUE : has_data, newdata);
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

StructVal *StructVal::create(Context &c, Type *ty, ContainsData has_data,
			     const std::unordered_map<std::string, Value *> &val)
{
	return c.allocVal<StructVal>(ty, has_data, val);
}

FuncVal::FuncVal(Context &c, FuncTy *val) : Value(VFUNC, val, CDTRUE) {}

std::string FuncVal::toStr()
{
	return "func<" + ty->toStr() + ">";
}
Value *FuncVal::clone(Context &c)
{
	return create(c, as<FuncTy>(ty->clone(c)));
}
bool FuncVal::updateValue(Context &c, Value *v)
{
	return true;
}

FuncVal *FuncVal::create(Context &c, FuncTy *val)
{
	return c.allocVal<FuncVal>(val);
}

TypeVal::TypeVal(Context &c, Type *val) : Value(VTYPE, val, CDTRUE) {}

std::string TypeVal::toStr()
{
	return "typeval<" + ty->toStr() + ">";
}
Value *TypeVal::clone(Context &c)
{
	return create(c, ty->clone(c));
}
bool TypeVal::updateValue(Context &c, Value *v)
{
	return true;
}

TypeVal *TypeVal::create(Context &c, Type *val)
{
	return c.allocVal<TypeVal>(val);
}

ImportVal::ImportVal(Context &c, const std::string &val)
	: Value(VIMPORT, mkStrTy(c), CDTRUE), val(val)
{}

std::string ImportVal::toStr()
{
	return val;
}
Value *ImportVal::clone(Context &c)
{
	return create(c, val);
}
bool ImportVal::updateValue(Context &c, Value *v)
{
	if(!v->isImport()) return false;
	val	 = as<ImportVal>(v)->getVal();
	has_data = v->getHasData() == CDTRUE || v->getHasData() == CDPERMA ? CDTRUE : CDFALSE;
	return true;
}

ImportVal *ImportVal::create(Context &c, const std::string &val)
{
	return c.allocVal<ImportVal>(val);
}

RefVal::RefVal(Context &c, Type *ty, Value *to) : Value(VREF, ty, CDTRUE), to(to) {}

std::string RefVal::toStr()
{
	return "ref<" + to->toStr() + ">";
}
Value *RefVal::clone(Context &c)
{
	return create(c, ty, to->clone(c));
}
bool RefVal::updateValue(Context &c, Value *v)
{
	if(!v->isRef()) return false;
	to->setHasData(v->getHasData());
	return to->updateValue(c, as<RefVal>(v)->getVal());
}

RefVal *RefVal::create(Context &c, Type *ty, Value *to)
{
	return c.allocVal<RefVal>(ty, to);
}

ContainsData RefVal::getHasData()
{
	return to->getHasData();
}
void RefVal::setHasData(ContainsData cd)
{
	to->setHasData(cd);
}
void RefVal::setContainsData()
{
	to->setContainsData();
}
void RefVal::setContainsPermaData()
{
	to->setContainsPermaData();
}
void RefVal::unsetContainsPermaData()
{
	to->unsetContainsPermaData();
}
bool RefVal::hasData()
{
	return to->hasData();
}
bool RefVal::hasPermaData()
{
	return to->hasPermaData();
}
void RefVal::clearHasData()
{
	to->clearHasData();
}
} // namespace sc
