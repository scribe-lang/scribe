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
	VVOID,
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

	virtual std::string toStr()	 = 0;
	virtual Value *clone(Context &c) = 0;

	inline Values getType() const
	{
		return type;
	}
};

template<typename T> T *as(Value *v)
{
	return static_cast<T *>(v);
}

class VoidVal : public Value
{
public:
	VoidVal();

	std::string toStr();
	Value *clone(Context &c);

	static VoidVal *create(Context &c);
};

class IntVal : public Value
{
	int64_t data;

public:
	IntVal(const int64_t &data);

	std::string toStr();
	Value *clone(Context &c);

	static IntVal *create(Context &c, const int64_t &val);

	inline int64_t &getVal()
	{
		return data;
	}
};

class FltVal : public Value
{
	long double data;

public:
	FltVal(const long double &data);

	std::string toStr();
	Value *clone(Context &c);

	static FltVal *create(Context &c, const long double &val);

	inline long double &getVal()
	{
		return data;
	}
};

class VecVal : public Value
{
	std::vector<Value *> data;

public:
	VecVal(const std::vector<Value *> &data);

	std::string toStr();
	Value *clone(Context &c);

	static VecVal *create(Context &c, const std::vector<Value *> &val);
	static VecVal *createStr(Context &c, const std::string &val);

	inline std::vector<Value *> &getVal()
	{
		return data;
	}
	std::string getAsString();
};

class StructVal : public Value
{
	std::unordered_map<std::string, Value *> data;

public:
	StructVal(const std::unordered_map<std::string, Value *> &data);

	std::string toStr();
	Value *clone(Context &c);

	static StructVal *create(Context &c, const std::unordered_map<std::string, Value *> &val);

	inline std::unordered_map<std::string, Value *> &getVal()
	{
		return data;
	}

	inline Value *getValAttr(const std::string &key)
	{
		return data[key];
	}
};
} // namespace sc

#endif // VALUES_HPP