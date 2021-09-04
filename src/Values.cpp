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

IntVal::IntVal(const int64_t &data) : Value(VINT), data(data) {}

IntVal *IntVal::create(Context &c, const int64_t &val)
{
	return c.allocVal<IntVal>(val);
}

FltVal::FltVal(const long double &data) : Value(VFLT), data(data) {}

FltVal *FltVal::create(Context &c, const long double &val)
{
	return c.allocVal<FltVal>(val);
}

VecVal::VecVal(const std::vector<Value *> &data) : Value(VVEC), data(data) {}

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

StructVal::StructVal(const std::unordered_map<std::string, Value *> &data)
	: Value(VSTRUCT), data(data)
{}

StructVal *StructVal::create(Context &c, const std::unordered_map<std::string, Value *> &val)
{
	return c.allocVal<StructVal>(val);
}
} // namespace sc
