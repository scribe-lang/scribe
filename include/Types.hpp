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

	bool isBaseCompatible(Context &c, Type *rhs, ErrMgr &e, const ModuleLoc *loc);

	std::string infoToStr();
	std::string baseToStr();
	bool requiresCast(Type *other);

	virtual uint64_t getUniqID(); // used by codegen
	virtual uint64_t getID();
	virtual bool isTemplate(const size_t &weak_depth = 0);
	virtual std::string toStr(const size_t &weak_depth = 0);
	virtual Type *clone(Context &c, const bool &as_is = false,
			    const size_t &weak_depth = 0) = 0;
	virtual bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	virtual void unmergeTemplates(const size_t &weak_depth = 0);
	virtual bool isCompatible(Context &c, Type *rhs, ErrMgr &e, const ModuleLoc *loc);

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

	virtual Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
				      const size_t &weak_depth = 0);

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

	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);

	static VoidTy *create(Context &c);

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};
class AnyTy : public Type
{
public:
	AnyTy();
	AnyTy(const size_t &info);
	~AnyTy();

	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);

	static AnyTy *create(Context &c);

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
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
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);

	static IntTy *create(Context &c, const size_t &_bits, const bool &_sign);

	inline const size_t &getBits() const
	{
		return bits;
	}
	inline const bool &isSigned() const
	{
		return sign;
	}

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class FltTy : public Type
{
	size_t bits;

public:
	FltTy(const size_t &bits);
	FltTy(const size_t &info, const uint64_t &id, const size_t &bits);
	~FltTy();

	uint64_t getID();
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);

	static FltTy *create(Context &c, const size_t &_bits);

	inline const size_t &getBits() const
	{
		return bits;
	}

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class TypeTy : public Type
{
	uint64_t containedtyid;

public:
	TypeTy();
	TypeTy(const size_t &info, const uint64_t &id, const uint64_t &containedtyid);
	~TypeTy();

	uint64_t getUniqID();
	bool isTemplate(const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);

	static TypeTy *create(Context &c);

	void clearContainedTy();
	void setContainedTy(Type *ty);
	Type *getContainedTy();

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class PtrTy : public Type
{
	Type *to;
	size_t count; // 0 = normal pointer, > 0 = array pointer with count = size
	bool is_weak; // required for self referencing members in struct

public:
	PtrTy(Type *to, const size_t &count, const bool &is_weak);
	PtrTy(const size_t &info, const uint64_t &id, Type *to, const size_t &count,
	      const bool &is_weak);
	~PtrTy();

	uint64_t getUniqID();
	bool isTemplate(const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);

	static PtrTy *create(Context &c, Type *ptr_to, const size_t &count, const bool &is_weak);

	inline void setTo(Type *ty)
	{
		to = ty;
	}
	inline void setWeak(const bool &weak)
	{
		is_weak = weak;
	}
	inline Type *&getTo()
	{
		return to;
	}
	inline const size_t &getCount()
	{
		return count;
	}
	inline bool isWeak()
	{
		return is_weak;
	}
	inline bool isArrayPtr()
	{
		return count > 0;
	}

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
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

	uint64_t getUniqID();
	bool isTemplate(const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, ErrMgr &e, const ModuleLoc *loc);
	// specializes a structure type
	StructTy *applyTemplates(Context &c, ErrMgr &e, const ModuleLoc *loc,
				 const std::vector<Type *> &actuals);
	// returns a NON-def struct type
	StructTy *instantiate(Context &c, ErrMgr &e, const ModuleLoc *loc,
			      const std::vector<Stmt *> &callargs);

	static StructTy *create(Context &c, const std::vector<std::string> &_fieldnames,
				const std::vector<Type *> &_fields,
				const std::vector<std::string> &_templatenames,
				const std::vector<TypeTy *> &_templates, const bool &_externed);

	inline void insertField(const std::string &name, Type *ty)
	{
		fieldpos[name] = fields.size();
		fieldnames.push_back(name);
		fields.push_back(ty);
	}
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
	inline const std::vector<TypeTy *> &getTemplates()
	{
		return templates;
	}
	inline const std::vector<std::string> &getTemplateNames()
	{
		return templatenames;
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

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
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

	// returns ID of parameters + ret type
	uint64_t getSignatureID();
	uint64_t getNonUniqID();
	uint64_t getID();
	bool isTemplate(const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, ErrMgr &e, const ModuleLoc *loc);
	// specializes a function type using StmtFnCallInfo
	FuncTy *createCall(Context &c, ErrMgr &e, const ModuleLoc *loc,
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
	inline void setRet(Type *retty)
	{
		ret = retty;
	}
	inline void insertArg(Type *arg)
	{
		args.push_back(arg);
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

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class VariadicTy : public Type
{
	std::vector<Type *> args;

public:
	VariadicTy(const std::vector<Type *> &args);
	VariadicTy(const size_t &info, const uint64_t &id, const std::vector<Type *> &args);
	~VariadicTy();

	bool isTemplate(const size_t &weak_depth = 0);
	std::string toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, ErrMgr &e, const ModuleLoc *loc);

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

	Value *toDefaultValue(Context &c, ErrMgr &e, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

// helpful functions

inline IntTy *mkI0Ty(Context &c)
{
	return IntTy::create(c, 0, true);
}
inline IntTy *mkI1Ty(Context &c)
{
	return IntTy::create(c, 1, true);
}
inline IntTy *mkI8Ty(Context &c)
{
	return IntTy::create(c, 8, true);
}
inline IntTy *mkI16Ty(Context &c)
{
	return IntTy::create(c, 16, true);
}
inline IntTy *mkI32Ty(Context &c)
{
	return IntTy::create(c, 32, true);
}
inline IntTy *mkI64Ty(Context &c)
{
	return IntTy::create(c, 64, true);
}
inline IntTy *mkU8Ty(Context &c)
{
	return IntTy::create(c, 8, false);
}
inline IntTy *mkU16Ty(Context &c)
{
	return IntTy::create(c, 16, false);
}
inline IntTy *mkU32Ty(Context &c)
{
	return IntTy::create(c, 32, false);
}
inline IntTy *mkU64Ty(Context &c)
{
	return IntTy::create(c, 64, false);
}
inline FltTy *mkF0Ty(Context &c)
{
	return FltTy::create(c, 0);
}
inline FltTy *mkF32Ty(Context &c)
{
	return FltTy::create(c, 32);
}
inline FltTy *mkF64Ty(Context &c)
{
	return FltTy::create(c, 64);
}
inline PtrTy *mkPtrTy(Context &c, Type *to, const size_t &count, const bool &is_weak)
{
	return PtrTy::create(c, to, count, is_weak);
}
inline Type *mkStrTy(Context &c)
{
	Type *res = IntTy::create(c, 8, true);
	res	  = PtrTy::create(c, res, 0, false);
	res->setConst();
	return res;
}
inline VoidTy *mkVoidTy(Context &c)
{
	return VoidTy::create(c);
}

inline AnyTy *mkAnyTy(Context &c)
{
	return AnyTy::create(c);
}
inline TypeTy *mkTypeTy(Context &c)
{
	return TypeTy::create(c);
}
} // namespace sc

#endif // TYPES_HPP