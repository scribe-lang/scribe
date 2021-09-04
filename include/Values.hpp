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

#ifndef VALUES_HPP
#define VALUES_HPP

#include <cinttypes>
#include <cstddef>
#include <string>
#include <unordered_map>

#include "Context.hpp"

namespace sc
{
enum Values
{
	VINT,
	VFLT,
	VVEC,
	VSTRUCT,
};

class Value
{
	Values type;

public:
	Value(const Values &type);
	virtual ~Value();
};

class IntVal : public Value
{
	int64_t data;

public:
	IntVal(const int64_t &data);

	static IntVal *create(Context &c, const int64_t &val);

	inline const int64_t &getVal() const
	{
		return data;
	}
};

class FltVal : public Value
{
	long double data;

public:
	FltVal(const long double &data);

	static FltVal *create(Context &c, const long double &val);

	inline const long double &getVal() const
	{
		return data;
	}
};

class VecVal : public Value
{
	std::vector<Value *> data;

public:
	VecVal(const std::vector<Value *> &data);

	static VecVal *create(Context &c, const std::vector<Value *> &val);
	static VecVal *createStr(Context &c, const std::string &val);

	inline const std::vector<Value *> &getVal() const
	{
		return data;
	}
};

class StructVal : public Value
{
	std::unordered_map<std::string, Value *> data;

public:
	StructVal(const std::unordered_map<std::string, Value *> &data);

	static StructVal *create(Context &c, const std::unordered_map<std::string, Value *> &val);

	inline const std::unordered_map<std::string, Value *> &getVal() const
	{
		return data;
	}

	inline const Value *getValAttr(const std::string &key)
	{
		return data[key];
	}
};
} // namespace sc

#endif // VALUES_HPP