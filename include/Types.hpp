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
#include "Values.hpp"

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

class Stmt;
class StmtExpr;
class StmtVar;
class StmtFnDef;
class StmtFnCallInfo;
typedef bool (*IntrinsicFn)(Context &c, ErrMgr &err, StmtExpr *stmt, Stmt **source,
			    std::vector<Stmt *> &args);
#define INTRINSIC(name)                                                               \
	bool intrinsic_##name(Context &c, ErrMgr &err, StmtExpr *stmt, Stmt **source, \
			      std::vector<Stmt *> &args)

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
	bool requiresCast(Type *other);

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

	virtual Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);

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

class VoidTy : public Type
{
public:
	VoidTy();
	VoidTy(const size_t &info);
	~VoidTy();

	Type *clone(Context &c, const bool &as_is = false);
	std::string toStr();

	static VoidTy *create(Context &c);

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
};
class AnyTy : public Type
{
public:
	AnyTy();
	AnyTy(const size_t &info);
	~AnyTy();

	Type *clone(Context &c, const bool &as_is = false);
	std::string toStr();

	static AnyTy *create(Context &c);

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
};

class IntTy : public Type
{
	size_t bits;
	bool sign; // signed

public:
	IntTy(const size_t &bits, const bool &sign);
	IntTy(const size_t &info, const uint64_t &id, const size_t &bits, const bool &sign);
	~IntTy();

	uint64_t getID();
	Type *clone(Context &c, const bool &as_is = false);
	std::string toStr();

	static IntTy *create(Context &c, const size_t &_bits, const bool &_sign);

	inline const size_t &getBits() const
	{
		return bits;
	}
	inline const bool &isSigned() const
	{
		return sign;
	}

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
};

class FltTy : public Type
{
	size_t bits;

public:
	FltTy(const size_t &bits);
	FltTy(const size_t &info, const uint64_t &id, const size_t &bits);
	~FltTy();

	uint64_t getID();
	Type *clone(Context &c, const bool &as_is = false);
	std::string toStr();

	static FltTy *create(Context &c, const size_t &_bits);

	inline const size_t &getBits() const
	{
		return bits;
	}

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
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

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
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

	inline Type *&getTo()
	{
		return to;
	}
	inline const size_t &getCount()
	{
		return count;
	}
	inline bool isArrayPtr()
	{
		return count > 0;
	}

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
};

class StructTy : public Type
{
	std::unordered_map<std::string, size_t> fieldpos;
	std::vector<std::string> fieldnames;
	std::vector<Type *> fields;
	std::unordered_map<std::string, size_t> templatepos;
	std::vector<std::string> templatenames;
	std::vector<TypeTy *> templates;
	bool has_template;
	bool externed;

public:
	StructTy(const std::vector<std::string> &fieldnames, const std::vector<Type *> &fields,
		 const std::vector<std::string> &templatenames,
		 const std::vector<TypeTy *> &templates, const bool &externed);
	StructTy(const size_t &info, const uint64_t &id, const std::vector<std::string> &fieldnames,
		 const std::unordered_map<std::string, size_t> &fieldpos,
		 const std::vector<Type *> &fields, const std::vector<std::string> &templatenames,
		 const std::unordered_map<std::string, size_t> &templatepos,
		 const std::vector<TypeTy *> &templates, const bool &has_template,
		 const bool &externed);
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
				const std::vector<TypeTy *> &_templates, const bool &_externed);

	inline void setExterned(const bool &ext)
	{
		externed = ext;
	}
	inline const std::string &getFieldName(const size_t &idx)
	{
		return fieldnames[idx];
	}
	inline std::vector<Type *> &getFields()
	{
		return fields;
	}
	inline std::vector<TypeTy *> &getTemplates()
	{
		return templates;
	}
	inline void clearTemplates()
	{
		templates.clear();
	}
	inline void setTemplate(const bool &has_templ)
	{
		has_template = has_templ;
	}
	inline bool isExtern()
	{
		return externed;
	}

	Type *getField(const std::string &name);
	Type *getField(const size_t &pos);

	bool hasTemplate();

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
};

enum IntrinType
{
	INONE,
	IPARSE,
	IVALUE,
};
class FuncTy : public Type
{
	StmtVar *var;
	std::vector<Type *> args;
	Type *ret;
	IntrinsicFn intrin;
	IntrinType inty;
	uint64_t uniqid;
	bool externed;

public:
	FuncTy(StmtVar *var, const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const IntrinType &inty, const bool &externed);
	FuncTy(const size_t &info, const uint64_t &id, StmtVar *var,
	       const std::vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const IntrinType &inty, const uint64_t &uniqid, const bool &externed);
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
			      Type *_ret, IntrinsicFn _intrin, const IntrinType &_inty,
			      const bool &_externed);

	inline void setVar(StmtVar *v)
	{
		var = v;
	}
	inline void setArg(const size_t &idx, Type *arg)
	{
		args[idx] = arg;
	}
	inline void insertArg(const size_t &idx, Type *arg)
	{
		args.insert(args.begin() + idx, arg);
	}
	inline void eraseArg(const size_t &idx)
	{
		args.erase(args.begin() + idx);
	}
	inline void setExterned(const bool &ext)
	{
		externed = ext;
	}
	inline StmtVar *&getVar()
	{
		return var;
	}
	inline std::vector<Type *> &getArgs()
	{
		return args;
	}
	inline Type *getArg(const size_t &idx)
	{
		return args.size() > idx ? args[idx] : nullptr;
	}
	inline Type *getRet()
	{
		return ret;
	}
	inline bool isIntrinsic()
	{
		return intrin != nullptr;
	}
	inline bool isParseIntrinsic()
	{
		return inty == IPARSE;
	}
	inline bool isExtern()
	{
		return externed;
	}
	inline IntrinsicFn getIntrinsicFn()
	{
		return intrin;
	}
	void updateUniqID();
	bool callIntrinsic(Context &c, ErrMgr &err, StmtExpr *stmt, Stmt **source,
			   std::vector<Stmt *> &callargs);

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
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

	inline void addArg(Type *ty)
	{
		args.push_back(ty);
	}
	inline std::vector<Type *> &getArgs()
	{
		return args;
	}
	inline Type *getArg(const size_t &idx)
	{
		return args.size() > idx ? args[idx] : nullptr;
	}

	Value *toDefaultValue(Context &c, ErrMgr &e, ModuleLoc &loc, ContainsData cd);
};

// helpful functions

inline Type *mkI1Ty(Context &c)
{
	return IntTy::create(c, 1, true);
}
inline Type *mkI8Ty(Context &c)
{
	return IntTy::create(c, 8, true);
}
inline Type *mkI16Ty(Context &c)
{
	return IntTy::create(c, 16, true);
}
inline Type *mkI32Ty(Context &c)
{
	return IntTy::create(c, 32, true);
}
inline Type *mkI64Ty(Context &c)
{
	return IntTy::create(c, 64, true);
}
inline Type *mkU8Ty(Context &c)
{
	return IntTy::create(c, 8, false);
}
inline Type *mkU16Ty(Context &c)
{
	return IntTy::create(c, 16, false);
}
inline Type *mkU32Ty(Context &c)
{
	return IntTy::create(c, 32, false);
}
inline Type *mkU64Ty(Context &c)
{
	return IntTy::create(c, 64, false);
}
inline Type *mkF32Ty(Context &c)
{
	return FltTy::create(c, 32);
}
inline Type *mkF64Ty(Context &c)
{
	return FltTy::create(c, 64);
}
inline Type *mkPtrTy(Context &c, Type *to, const size_t &count)
{
	return PtrTy::create(c, to, count);
}
inline Type *mkStrTy(Context &c)
{
	Type *res = IntTy::create(c, 8, true);
	res->setConst();
	res = PtrTy::create(c, res, 0);
	return res;
}

} // namespace sc

#endif // TYPES_HPP