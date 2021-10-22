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
Value::Value(const Values &type) : type(type) {}
Value::~Value() {}

VoidVal::VoidVal() : Value(VVOID) {}

std::string VoidVal::toStr()
{
	return "<void>";
}
Value *VoidVal::clone(Context &c)
{
	return create(c);
}

VoidVal *VoidVal::create(Context &c)
{
	return c.allocVal<VoidVal>();
}

IntVal::IntVal(const int64_t &data) : Value(VINT), data(data) {}

std::string IntVal::toStr()
{
	return std::to_string(data);
}
Value *IntVal::clone(Context &c)
{
	return create(c, data);
}

IntVal *IntVal::create(Context &c, const int64_t &val)
{
	return c.allocVal<IntVal>(val);
}

FltVal::FltVal(const long double &data) : Value(VFLT), data(data) {}

std::string FltVal::toStr()
{
	return std::to_string(data);
}
Value *FltVal::clone(Context &c)
{
	return create(c, data);
}

FltVal *FltVal::create(Context &c, const long double &val)
{
	return c.allocVal<FltVal>(val);
}

VecVal::VecVal(const std::vector<Value *> &data) : Value(VVEC), data(data) {}

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
	return create(c, newdata);
}

VecVal *VecVal::create(Context &c, const std::vector<Value *> &val)
{
	return c.allocVal<VecVal>(val);
}
VecVal *VecVal::createStr(Context &c, const std::string &val)
{
	std::vector<Value *> chars;
	for(auto &ch : val) {
		chars.push_back(IntVal::create(c, ch));
	}
	return c.allocVal<VecVal>(chars);
}

std::string VecVal::getAsString()
{
	std::string res;
	for(auto &ch : data) {
		if(ch->getType() != VINT) return "";
		res.push_back(as<IntVal>(ch)->getVal());
	}
	return res;
}

StructVal::StructVal(const std::unordered_map<std::string, Value *> &data)
	: Value(VSTRUCT), data(data)
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
	return create(c, newdata);
}

StructVal *StructVal::create(Context &c, const std::unordered_map<std::string, Value *> &val)
{
	return c.allocVal<StructVal>(val);
}
} // namespace sc
