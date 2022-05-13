/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef VALUES_HPP
#define VALUES_HPP

#include "Core.hpp"

namespace sc
{
enum Values : uint8_t
{
	VVOID,
	VINT,
	VFLT,
	VVEC,
	VSTRUCT,
	VFUNC,
	VTYPE, // value is type
	VNAMESPACE,
	VREF,
};
enum ContainsData : uint8_t
{
	CDFALSE, // no value is contained
	CDTRUE,	 // value is contained
	CDPERMA, // value is contained and is permanent
};

class Context;
class FuncTy;
class Type;

class Value
{
protected:
	Values vty;
	ContainsData has_data;
	Type *ty;

public:
	Value(const Values &vty, Type *ty, ContainsData has_data);
	virtual ~Value();

	virtual String toStr()			       = 0;
	virtual Value *clone(Context &c)	       = 0;
	virtual bool updateValue(Context &c, Value *v) = 0;

#define IsVal(ty, vt)                \
	inline bool is##ty()         \
	{                            \
		return vty == V##vt; \
	}
	IsVal(Void, VOID);
	IsVal(Int, INT);
	IsVal(Flt, FLT);
	IsVal(Vec, VEC);
	IsVal(Struct, STRUCT);
	IsVal(Func, FUNC);
	IsVal(Type, TYPE);
	IsVal(Namespace, NAMESPACE);
	IsVal(Ref, REF);

	bool isStrLiteral();

	inline void setType(Type *t)
	{
		ty = t;
	}
	inline Values getValType()
	{
		return vty;
	}
	inline Type *&getType()
	{
		return ty;
	}
	virtual ContainsData getHasData();
	virtual void setHasData(ContainsData cd);
	virtual void setContainsData();
	virtual void setContainsPermaData();
	virtual void unsetContainsPermaData();
	virtual bool hasData();
	virtual bool hasPermaData();
	virtual void clearHasData();
};

template<typename T> T *as(Value *v)
{
	return static_cast<T *>(v);
}

class VoidVal : public Value
{
public:
	VoidVal(Context &c);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static VoidVal *create(Context &c);
};

class IntVal : public Value
{
	int64_t data;

public:
	IntVal(Context &c, Type *ty, ContainsData has_data, int64_t data);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static IntVal *create(Context &c, Type *ty, ContainsData has_data, int64_t val);

	inline int64_t &getVal()
	{
		return data;
	}
};

class FltVal : public Value
{
	long double data;

public:
	FltVal(Context &c, Type *ty, ContainsData has_data, const long double &data);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static FltVal *create(Context &c, Type *ty, ContainsData has_data, const long double &val);

	inline long double &getVal()
	{
		return data;
	}
};

class VecVal : public Value
{
	Vector<Value *> data;

public:
	VecVal(Context &c, Type *ty, ContainsData has_data, const Vector<Value *> &data);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static VecVal *create(Context &c, Type *ty, ContainsData has_data,
			      const Vector<Value *> &val);
	static VecVal *createStr(Context &c, StringRef val, ContainsData has_data);

	inline void insertVal(Value *v)
	{
		data.push_back(v);
	}
	inline Vector<Value *> &getVal()
	{
		return data;
	}
	inline Value *&getValAt(const size_t &idx)
	{
		return data[idx];
	}
	String getAsString();
};

class StructVal : public Value
{
	Map<StringRef, Value *> data;

public:
	StructVal(Context &c, Type *ty, ContainsData has_data, const Map<StringRef, Value *> &data);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static StructVal *create(Context &c, Type *ty, ContainsData has_data,
				 const Map<StringRef, Value *> &val);

	inline Map<StringRef, Value *> &getVal()
	{
		return data;
	}

	inline Value *getField(StringRef key)
	{
		if(data.find(key) == data.end()) return nullptr;
		return data[key];
	}
};

class FuncVal : public Value
{
public:
	FuncVal(Context &c, FuncTy *val);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static FuncVal *create(Context &c, FuncTy *val);

	inline FuncTy *getVal()
	{
		return (FuncTy *)ty;
	}
};

class TypeVal : public Value
{
public:
	TypeVal(Context &c, Type *val);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static TypeVal *create(Context &c, Type *val);

	inline void setVal(Type *v)
	{
		ty = v;
	}
	inline Type *&getVal()
	{
		return ty;
	}
};

class NamespaceVal : public Value
{
	StringRef val;

public:
	NamespaceVal(Context &c, StringRef val);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static NamespaceVal *create(Context &c, StringRef val);

	inline StringRef getVal()
	{
		return val;
	}
};

// refers to other values, generally used for updating types but work with same value
// for example - address of and dereference operators
class RefVal : public Value
{
	Value *to;

public:
	RefVal(Context &c, Type *ty, Value *to);

	String toStr();
	Value *clone(Context &c);
	bool updateValue(Context &c, Value *v);

	static RefVal *create(Context &c, Type *ty, Value *to);

	inline Value *&getVal()
	{
		return to;
	}

	ContainsData getHasData();
	void setHasData(ContainsData cd);
	void setContainsData();
	void setContainsPermaData();
	void unsetContainsPermaData();
	bool hasData();
	bool hasPermaData();
	void clearHasData();
};
} // namespace sc

#endif // VALUES_HPP