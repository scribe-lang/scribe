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
	COMPTIME = 1 << 4, // is comptime
	VARIADIC = 1 << 5, // is variadic
};

enum IntrinType
{
	INONE,

	IPARSE,
	IVALUE,

	IALL,
};

class Stmt;
class StmtExpr;
class StmtVar;
class StmtFnDef;
class StmtFnCallInfo;
class Value;
typedef bool (*IntrinsicFn)(Context &c, ErrMgr &err, StmtExpr *stmt, Stmt **source,
			    std::vector<Stmt *> &args, const IntrinType &currintrin);
#define INTRINSIC(name)                                                               \
	bool intrinsic_##name(Context &c, ErrMgr &err, StmtExpr *stmt, Stmt **source, \
			      std::vector<Stmt *> &args, const IntrinType &currintrin)

class Type
{
	uint64_t id;
	Types type;
	size_t info;

public:
	Type(const Types &type, const size_t &info, const uint64_t &id);
	virtual ~Type();

	bool isBaseCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc);

	std::string infoToStr();
	std::string baseToStr();

	virtual uint64_t getID();
	virtual bool isTemplate();
	virtual std::string toStr();
	virtual Type *clone(Context &c, const bool &as_is = false) = 0;
	virtual bool isCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc);

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
	inline bool isPrimitiveOrPtr() const
	{
		return isInt() || isFlt() || isPtr();
	}
	inline bool isIntegral() const
	{
		return isInt();
	}
	inline bool isFloat() const
	{
		return isFlt();
	}

	virtual Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc);

#define SetModifierX(Fn, Mod) \
	inline void set##Fn() \
	{                     \
		info |= Mod;  \
	}
	SetModifierX(Static, STATIC);
	SetModifierX(Const, CONST);
	SetModifierX(Volatile, VOLATILE);
	SetModifierX(Ref, REF);
	SetModifierX(Comptime, COMPTIME);
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
	UnsetModifierX(Comptime, COMPTIME);
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
	IsModifierX(Comptime, COMPTIME);
	IsModifierX(Variadic, VARIADIC);

	inline const uint64_t &getBaseID() const
	{
		return id;
	}
};

template<typename T> T *as(Type *t)
{
	return static_cast<T *>(t);
}

#define BasicTypeDecl(Ty)                                           \
	class Ty : public Type                                      \
	{                                                           \
	public:                                                     \
		Ty();                                               \
		Ty(const size_t &info);                             \
		~Ty();                                              \
		Type *clone(Context &c, const bool &as_is = false); \
		static Ty *create(Context &c);                      \
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

	Type *clone(Context &c, const bool &as_is = false);
	std::string toStr();

	static IntTy *create(Context &c, const size_t &_bits, const bool &_sign);

	const size_t &getBits() const;
	const bool &isSigned() const;

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc);
};

class FltTy : public Type
{
	size_t bits;

public:
	FltTy(const size_t &bits);
	FltTy(const size_t &info, const uint64_t &id, const size_t &bits);
	~FltTy();

	Type *clone(Context &c, const bool &as_is = false);
	std::string toStr();

	static FltTy *create(Context &c, const size_t &_bits);

	const size_t &getBits() const;

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc);
};

class TypeTy : public Type
{
	uint64_t containedtyid;

public:
	TypeTy();
	TypeTy(const size_t &info, const uint64_t &id, const uint64_t &containedtyid);
	~TypeTy();

	bool isTemplate();
	std::string toStr();
	Type *clone(Context &c, const bool &as_is = false);

	static TypeTy *create(Context &c);

	void clearContainedTy();
	void setContainedTy(Type *ty);
	Type *getContainedTy();

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc);
};

class PtrTy : public Type
{
	Type *to;
	size_t count; // 0 = normal pointer, > 0 = array pointer with count = size

public:
	PtrTy(Type *to, const size_t &count);
	PtrTy(const size_t &info, const uint64_t &id, Type *to, const size_t &count);
	~PtrTy();

	bool isTemplate();
	std::string toStr();
	Type *clone(Context &c, const bool &as_is = false);

	static PtrTy *create(Context &c, Type *ptr_to, const size_t &count);

	Type *&getTo();
	const size_t &getCount();
	bool isArrayPtr();

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc);
};

class StructTy : public Type
{
	std::unordered_map<std::string, size_t> fieldpos;
	std::vector<std::string> fieldnames;
	std::vector<Type *> fields;
	std::unordered_map<std::string, size_t> templatepos;
	std::vector<std::string> templatenames;
	std::vector<TypeTy *> templates;
	bool is_def; // true by default
	bool has_template;

public:
	StructTy(const std::vector<std::string> &fieldnames, const std::vector<Type *> &fields,
		 const std::vector<std::string> &templatenames,
		 const std::vector<TypeTy *> &templates);
	StructTy(const size_t &info, const uint64_t &id, const std::vector<std::string> &fieldnames,
		 const std::unordered_map<std::string, size_t> &fieldpos,
		 const std::vector<Type *> &fields, const std::vector<std::string> &templatenames,
		 const std::unordered_map<std::string, size_t> &templatepos,
		 const std::vector<TypeTy *> &templates, const bool &is_def,
		 const bool &has_template);
	~StructTy();

	bool isTemplate();
	std::string toStr();
	Type *clone(Context &c, const bool &as_is = false);
	bool isCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc);
	// specializes a structure type
	StructTy *applyTemplates(Context &c, ErrMgr &e, ModuleLoc &loc,
				 const std::vector<Type *> &actuals);
	// returns a NON-def struct type
	StructTy *instantiate(Context &c, ErrMgr &e, ModuleLoc &loc,
			      const std::vector<Stmt *> &callargs);

	static StructTy *create(Context &c, const std::vector<std::string> &_fieldnames,
				const std::vector<Type *> &_fields,
				const std::vector<std::string> &_templatenames,
				const std::vector<TypeTy *> &_templates);

	const std::string &getFieldName(const size_t &idx);
	std::vector<Type *> &getFields();
	Type *getField(const std::string &name);
	Type *getField(const size_t &pos);
	std::vector<TypeTy *> &getTemplates();
	void clearTemplates();

	void setDef(const bool &def);
	bool isDef() const;
	void setTemplate(const bool &has_templ);
	bool hasTemplate();

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc);
};

class FuncTy : public Type
{
	StmtVar *var;
	std::vector<Type *> args;
	Type *ret;
	IntrinsicFn intrin;
	IntrinType intrinty;
	uint64_t uniqid;
	bool externed;

public:
	FuncTy(StmtVar *var, const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const IntrinType &intrinty, const bool &externed);
	FuncTy(StmtVar *var, const size_t &info, const uint64_t &id,
	       const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const IntrinType &intrinty, const uint64_t &uniqid, const bool &externed);
	~FuncTy();

	uint64_t getID();
	bool isTemplate();
	std::string toStr();
	Type *clone(Context &c, const bool &as_is = false);
	bool isCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc);
	// specializes a function type using StmtFnCallInfo
	FuncTy *createCall(Context &c, ErrMgr &e, ModuleLoc &loc,
			   const std::vector<Stmt *> &callargs);

	static FuncTy *create(Context &c, StmtVar *_var, const std::vector<Type *> &_args,
			      Type *_ret, IntrinsicFn _intrin, const IntrinType &_intrinty,
			      const bool &_externed);

	void setVar(StmtVar *v);
	void insertArg(const size_t &idx, Type *arg);
	void updateUniqID();
	void setExterned(const bool &ext);
	StmtVar *&getVar();
	std::vector<Type *> &getArgs();
	Type *getArg(const size_t &idx);
	Type *getRet();
	bool isIntrinsic();
	bool isIntrinsicParseOnly();
	bool isIntrinsicParse();
	bool isIntrinsicValue();
	const size_t &getTemplates() const;
	bool isExtern();
	IntrinsicFn getIntrinsicFn();
	IntrinType getIntrinsicType();
	bool callIntrinsic(Context &c, ErrMgr &err, StmtExpr *stmt, Stmt **source,
			   std::vector<Stmt *> &callargs, const IntrinType &currintrin);
};

class VariadicTy : public Type
{
	std::vector<Type *> args;

public:
	VariadicTy(const std::vector<Type *> &args);
	VariadicTy(const size_t &info, const uint64_t &id, const std::vector<Type *> &args);
	~VariadicTy();

	bool isTemplate();
	std::string toStr();
	Type *clone(Context &c, const bool &as_is = false);
	bool isCompatible(Context &c, Type *rhs, ErrMgr &e, ModuleLoc &loc);

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

	Type *clone(Context &c, const bool &as_is = false);
	std::string toStr();

	static ImportTy *create(Context &c, const std::string &_impid);

	const std::string &getImportID() const;
};
} // namespace sc

#endif // TYPES_HPP