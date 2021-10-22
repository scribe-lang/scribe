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

#ifndef TYPES_HPP
#define TYPES_HPP

#include <cinttypes>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Context.hpp"
#include "Error.hpp"

namespace sc
{
enum Types
{
	TVOID,
	// TNIL,  // = i1 (0)
	// TBOOL, // = i1
	TTYPE,
	TANY,

	TINT,
	TFLT,

	TPTR,
	TARRAY,
	TFUNC,
	TSTRUCT,
	TENUM,
	TVARIADIC,

	TIMPORT,

	_LAST
};

enum TypeInfoMask
{
	REF	 = 1 << 0, // is a reference
	STATIC	 = 1 << 1, // is static
	CONST	 = 1 << 2, // is const
	VOLATILE = 1 << 3, // is volatile
	VARIADIC = 1 << 4, // is variadic
};

class Stmt;
class StmtExpr;
class StmtFnCallInfo;
typedef bool (*IntrinsicFn)(Context &c, StmtExpr *stmt, Stmt **source, StmtFnCallInfo *callinfo);
#define INTRINSIC(name) \
	bool intrinsic##name(Context &c, StmtExpr *stmt, Stmt **source, StmtFnCallInfo *callinfo)

class Type
{
	uint64_t id;
	Types type;
	size_t info;

public:
	Type(const Types &type, const size_t &info, const uint64_t &id);
	virtual ~Type();

	bool isBaseCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc);

	std::string infoToStr();
	std::string baseToStr();

	virtual std::string toStr();
	virtual Type *clone(Context &c) = 0;
	virtual bool isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc);

	inline void setInfo(const size_t &inf)
	{
		info = inf;
	}
	inline void appendInfo(const size_t &inf)
	{
		info |= inf;
	}
	inline const size_t &getInfo() const
	{
		return info;
	}
	inline bool isPrimitive() const
	{
		return isInt() || isFlt();
	}
	inline bool isIntegral() const
	{
		return isInt();
	}
	inline bool isFloat() const
	{
		return isFlt();
	}

#define SetModifierX(Fn, Mod) \
	inline void set##Fn() \
	{                     \
		info |= Mod;  \
	}
	SetModifierX(Static, STATIC);
	SetModifierX(Const, CONST);
	SetModifierX(Volatile, VOLATILE);
	SetModifierX(Ref, REF);
	SetModifierX(Variadic, VARIADIC);

#define UnsetModifierX(Fn, Mod) \
	inline void unset##Fn() \
	{                       \
		info &= ~Mod;   \
	}
	UnsetModifierX(Static, STATIC);
	UnsetModifierX(Const, CONST);
	UnsetModifierX(Volatile, VOLATILE);
	UnsetModifierX(Ref, REF);
	UnsetModifierX(Variadic, VARIADIC);

#define IsTyX(Fn, Ty)                 \
	inline bool is##Fn() const    \
	{                             \
		return type == T##Ty; \
	}
	IsTyX(Void, VOID);
	IsTyX(TypeTy, TYPE);
	IsTyX(Any, ANY);
	IsTyX(Int, INT);
	IsTyX(Flt, FLT);
	IsTyX(Ptr, PTR);
	IsTyX(Array, ARRAY);
	IsTyX(Func, FUNC);
	IsTyX(Struct, STRUCT);
	IsTyX(Enum, ENUM);
	IsTyX(Variadic, VARIADIC);
	IsTyX(Import, IMPORT);

#define IsModifierX(Fn, Mod)        \
	inline bool has##Fn() const \
	{                           \
		return info & Mod;  \
	}
	IsModifierX(Static, STATIC);
	IsModifierX(Const, CONST);
	IsModifierX(Volatile, VOLATILE);
	IsModifierX(Ref, REF);
	IsModifierX(Variadic, VARIADIC);

	inline const uint64_t &getID() const
	{
		return id;
	}
};

template<typename T> T *as(Type *t)
{
	return static_cast<T *>(t);
}

#define BasicTypeDecl(Ty)                      \
	class Ty : public Type                 \
	{                                      \
	public:                                \
		Ty();                          \
		Ty(const size_t &info);        \
		~Ty();                         \
		Type *clone(Context &c);       \
		static Ty *create(Context &c); \
	}

BasicTypeDecl(VoidTy);
BasicTypeDecl(AnyTy);

class IntTy : public Type
{
	size_t bits;
	bool sign; // signed

public:
	IntTy(const size_t &bits, const bool &sign);
	IntTy(const size_t &info, const uint64_t &id, const size_t &bits, const bool &sign);
	~IntTy();

	Type *clone(Context &c);
	std::string toStr();

	static IntTy *create(Context &c, const size_t &_bits, const bool &_sign);

	const size_t &getBits() const;
	const bool &isSigned() const;
};

class FltTy : public Type
{
	size_t bits;

public:
	FltTy(const size_t &bits);
	FltTy(const size_t &info, const uint64_t &id, const size_t &bits);
	~FltTy();

	Type *clone(Context &c);
	std::string toStr();

	static FltTy *create(Context &c, const size_t &_bits);

	const size_t &getBits() const;
};

class TypeTy : public Type
{
	Type *containedty;

public:
	TypeTy(Type *containedty);
	TypeTy(const size_t &info, const uint64_t &id, Type *containedty);
	~TypeTy();

	Type *clone(Context &c);
	std::string toStr();

	static TypeTy *create(Context &c, Type *_containedty);

	Type *getContainedTy();
};

class PtrTy : public Type
{
	Type *to;

public:
	PtrTy(Type *to);
	PtrTy(const size_t &info, const uint64_t &id, Type *to);
	~PtrTy();

	Type *clone(Context &c);
	std::string toStr();

	static PtrTy *create(Context &c, Type *ptr_to);

	Type *getTo();
};

class ArrayTy : public Type
{
	uint64_t count;
	Type *of;

public:
	ArrayTy(const uint64_t &count, Type *of);
	ArrayTy(const size_t &info, const uint64_t &id, const uint64_t &count, Type *of);
	~ArrayTy();

	Type *clone(Context &c);
	std::string toStr();
	bool isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc);

	static ArrayTy *create(Context &c, const uint64_t &arr_count, Type *arr_of);

	const uint64_t &getCount();
	Type *getOf();
};

class StructTy : public Type
{
	std::unordered_map<std::string, size_t> fieldpos;
	std::vector<Type *> fields;
	bool is_def; // true by default

public:
	StructTy(const std::vector<std::string> &fieldnames, const std::vector<Type *> &fields);
	StructTy(const size_t &info, const uint64_t &id,
		 const std::unordered_map<std::string, size_t> &fieldpos,
		 const std::vector<Type *> &fields, const bool &is_def);
	~StructTy();

	Type *clone(Context &c);
	std::string toStr();
	bool isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc);
	// specializes a structure type using StmtFnCallInfo and returns a NON-def struct type
	StructTy *instantiate(Context &c, ErrMgr &e, StmtFnCallInfo *callinfo);

	static StructTy *create(Context &c, const std::vector<std::string> &_fieldnames,
				const std::vector<Type *> &_fields);

	std::vector<Type *> &getFields();
	Type *getField(const std::string &name);
	Type *getField(const size_t &pos);

	void setDef(const bool &def);
	bool isDef() const;
};

class FuncTy : public Type
{
	Stmt *def;
	std::vector<Type *> args;
	Type *ret;
	IntrinsicFn intrin;
	bool externed;

public:
	FuncTy(Stmt *def, const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const bool &externed);
	FuncTy(Stmt *def, const size_t &info, const uint64_t &id, const std::vector<Type *> &args,
	       Type *ret, IntrinsicFn intrin, const bool &externed);
	~FuncTy();

	Type *clone(Context &c);
	std::string toStr();
	bool isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc);
	// specializes a function type using StmtFnCallInfo
	FuncTy *createCall(Context &c, ErrMgr &e, StmtFnCallInfo *callinfo);

	static FuncTy *create(Context &c, Stmt *_def, const std::vector<Type *> &_args, Type *_ret,
			      IntrinsicFn _intrin, const bool &_externed);

	Stmt *&getDef();
	std::vector<Type *> &getArgs();
	Type *getArg(const size_t &idx);
	Type *getRet();
	bool isIntrinsic();
	const size_t &getTemplates() const;
	bool isExtern();
	bool callIntrinsic(Context &c, StmtExpr *stmt, Stmt **source, StmtFnCallInfo *callinfo);
};

class VariadicTy : public Type
{
	std::vector<Type *> args;

public:
	VariadicTy(const std::vector<Type *> &args);
	VariadicTy(const size_t &info, const uint64_t &id, const std::vector<Type *> &args);
	~VariadicTy();

	Type *clone(Context &c);
	std::string toStr();
	bool isCompatible(Type *rhs, ErrMgr &e, ModuleLoc &loc);

	static VariadicTy *create(Context &c, const std::vector<Type *> &_args);

	void addArg(Type *ty);
	std::vector<Type *> &getArgs();
	Type *getArg(const size_t &idx);
};

class ImportTy : public Type
{
	std::string impid;

public:
	ImportTy(const std::string &impid);
	ImportTy(const size_t &info, const uint64_t &id, const std::string &impid);
	~ImportTy();

	Type *clone(Context &c);
	std::string toStr();

	static ImportTy *create(Context &c, const std::string &_impid);

	const std::string &getImportID() const;
};
} // namespace sc

#endif // TYPES_HPP