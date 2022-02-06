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

#include "Context.hpp"
#include "Error.hpp"
#include "Values.hpp"

namespace sc
{
enum Types : uint16_t
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

// TODO:
// This enum (and respective variables) will be completely removed
// Only variadic will remain - inside FuncTy as a boolean
// All types will be allocated once, except func and struct -
// they must be allocated for each func/struct
enum TypeInfoMask
{
	REF	 = 1 << 0, // is a reference
	CONST	 = 1 << 1, // is const
	COMPTIME = 1 << 2, // is comptime
	VARIADIC = 1 << 3, // is variadic
};

class Stmt;
class StmtExpr;
class StmtVar;
class StmtFnDef;
class StmtFnCallInfo;
typedef bool (*IntrinsicFn)(Context &c, StmtExpr *stmt, Stmt **source, Vector<Stmt *> &args);
#define INTRINSIC(name) \
	bool intrinsic_##name(Context &c, StmtExpr *stmt, Stmt **source, Vector<Stmt *> &args)

class Type
{
	uint32_t id;
	Types type;
	uint16_t info;

public:
	Type(const Types &type, const uint16_t &info, const uint32_t &id);
	virtual ~Type();

	bool isBaseCompatible(Context &c, Type *rhs, const ModuleLoc *loc);

	String infoToStr();
	String baseToStr();
	bool requiresCast(Type *other);

	virtual uint32_t getUniqID(); // used by codegen
	virtual uint32_t getID();
	virtual bool isTemplate(const size_t &weak_depth = 0);
	virtual String toStr(const size_t &weak_depth = 0);
	virtual Type *clone(Context &c, const bool &as_is = false,
			    const size_t &weak_depth = 0) = 0;
	virtual bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	virtual void unmergeTemplates(const size_t &weak_depth = 0);
	virtual bool isCompatible(Context &c, Type *rhs, const ModuleLoc *loc);

	inline void setInfo(const size_t &inf)
	{
		info = inf;
	}
	inline void appendInfo(const size_t &inf)
	{
		info |= inf;
	}
	inline uint16_t getInfo() const
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

	virtual Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
				      const size_t &weak_depth = 0);

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
#undef IsTyX

#define SetModifierX(Fn, Mod) \
	inline void set##Fn() \
	{                     \
		info |= Mod;  \
	}
	SetModifierX(Const, CONST);
	SetModifierX(Ref, REF);
	SetModifierX(Comptime, COMPTIME);
	SetModifierX(Variadic, VARIADIC);
#undef SetModifierX

#define UnsetModifierX(Fn, Mod) \
	inline void unset##Fn() \
	{                       \
		info &= ~Mod;   \
	}
	UnsetModifierX(Const, CONST);
	UnsetModifierX(Ref, REF);
	UnsetModifierX(Comptime, COMPTIME);
	UnsetModifierX(Variadic, VARIADIC);
#undef UnsetModifierX

#define IsModifierX(Fn, Mod)        \
	inline bool has##Fn() const \
	{                           \
		return info & Mod;  \
	}
	IsModifierX(Const, CONST);
	IsModifierX(Ref, REF);
	IsModifierX(Comptime, COMPTIME);
	IsModifierX(Variadic, VARIADIC);
#undef IsModifierX

	inline const uint32_t &getBaseID() const
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
	VoidTy(const uint16_t &info);
	~VoidTy();

	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);

	static VoidTy *create(Context &c);

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};
class AnyTy : public Type
{
public:
	AnyTy();
	AnyTy(const uint16_t &info);
	~AnyTy();

	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);

	static AnyTy *create(Context &c);

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class IntTy : public Type
{
	uint16_t bits;
	bool sign; // signed

public:
	IntTy(const uint16_t &bits, const bool &sign);
	IntTy(const uint16_t &info, const uint32_t &id, const uint16_t &bits, const bool &sign);
	~IntTy();

	uint32_t getID();
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);

	static IntTy *create(Context &c, const uint16_t &_bits, const bool &_sign);

	inline const uint16_t &getBits() const
	{
		return bits;
	}
	inline const bool &isSigned() const
	{
		return sign;
	}

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class FltTy : public Type
{
	uint16_t bits;

public:
	FltTy(const uint16_t &bits);
	FltTy(const uint16_t &info, const uint32_t &id, const uint16_t &bits);
	~FltTy();

	uint32_t getID();
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);

	static FltTy *create(Context &c, const uint16_t &_bits);

	inline const uint16_t &getBits() const
	{
		return bits;
	}

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class TypeTy : public Type
{
	uint32_t containedtyid;

public:
	TypeTy();
	TypeTy(const uint16_t &info, const uint32_t &id, const uint32_t &containedtyid);
	~TypeTy();

	uint32_t getUniqID();
	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);

	static TypeTy *create(Context &c);

	void clearContainedTy();
	void setContainedTy(Type *ty);
	Type *getContainedTy();

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class PtrTy : public Type
{
	Type *to;
	uint16_t count; // 0 = normal pointer, > 0 = array pointer with count = size
	bool is_weak;	// required for self referencing members in struct

public:
	PtrTy(Type *to, const uint16_t &count, const bool &is_weak);
	PtrTy(const uint16_t &info, const uint32_t &id, Type *to, const uint16_t &count,
	      const bool &is_weak);
	~PtrTy();

	uint32_t getUniqID();
	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);

	static PtrTy *create(Context &c, Type *ptr_to, const uint16_t &count, const bool &is_weak);

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
	inline const uint16_t &getCount()
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

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class StructTy : public Type
{
	Map<StringRef, size_t> fieldpos;
	Vector<StringRef> fieldnames;
	Vector<Type *> fields;
	Map<StringRef, size_t> templatepos;
	Vector<StringRef> templatenames;
	Vector<TypeTy *> templates;
	bool has_template;
	bool externed;

public:
	StructTy(const Vector<StringRef> &fieldnames, const Vector<Type *> &fields,
		 const Vector<StringRef> &templatenames, const Vector<TypeTy *> &templates,
		 const bool &externed);
	StructTy(const uint16_t &info, const uint32_t &id, const Vector<StringRef> &fieldnames,
		 const Map<StringRef, size_t> &fieldpos, const Vector<Type *> &fields,
		 const Vector<StringRef> &templatenames, const Map<StringRef, size_t> &templatepos,
		 const Vector<TypeTy *> &templates, const bool &has_template, const bool &externed);
	~StructTy();

	uint32_t getUniqID();
	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, const ModuleLoc *loc);
	// specializes a structure type
	StructTy *applyTemplates(Context &c, const ModuleLoc *loc, const Vector<Type *> &actuals);
	// returns a NON-def struct type
	StructTy *instantiate(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &callargs);

	static StructTy *create(Context &c, const Vector<StringRef> &_fieldnames,
				const Vector<Type *> &_fields,
				const Vector<StringRef> &_templatenames,
				const Vector<TypeTy *> &_templates, const bool &_externed);

	inline void insertField(StringRef name, Type *ty)
	{
		fieldpos[name] = fields.size();
		fieldnames.push_back(name);
		fields.push_back(ty);
	}
	inline void setExterned(const bool &ext)
	{
		externed = ext;
	}
	inline StringRef getFieldName(const size_t &idx)
	{
		return fieldnames[idx];
	}
	inline Vector<Type *> &getFields()
	{
		return fields;
	}
	inline const Vector<TypeTy *> &getTemplates()
	{
		return templates;
	}
	inline const Vector<StringRef> &getTemplateNames()
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

	Type *getField(StringRef name);
	Type *getField(const size_t &pos);

	bool hasTemplate();

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
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
	Vector<Type *> args;
	Type *ret;
	IntrinsicFn intrin;
	IntrinType inty;
	uint32_t uniqid;
	bool externed;

public:
	FuncTy(StmtVar *var, const Vector<Type *> &args, Type *ret, IntrinsicFn intrin,
	       const IntrinType &inty, const bool &externed);
	FuncTy(const uint16_t &info, const uint32_t &id, StmtVar *var, const Vector<Type *> &args,
	       Type *ret, IntrinsicFn intrin, const IntrinType &inty, const uint32_t &uniqid,
	       const bool &externed);
	~FuncTy();

	// returns ID of parameters + ret type
	uint32_t getSignatureID();
	uint32_t getNonUniqID();
	uint32_t getID();
	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, const ModuleLoc *loc);
	// specializes a function type using StmtFnCallInfo
	FuncTy *createCall(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &callargs);

	static FuncTy *create(Context &c, StmtVar *_var, const Vector<Type *> &_args, Type *_ret,
			      IntrinsicFn _intrin, const IntrinType &_inty, const bool &_externed);

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
	inline Vector<Type *> &getArgs()
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
	bool callIntrinsic(Context &c, StmtExpr *stmt, Stmt **source, Vector<Stmt *> &callargs);

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class VariadicTy : public Type
{
	Vector<Type *> args;

public:
	VariadicTy(const Vector<Type *> &args);
	VariadicTy(const uint16_t &info, const uint32_t &id, const Vector<Type *> &args);
	~VariadicTy();

	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	Type *clone(Context &c, const bool &as_is = false, const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, const ModuleLoc *loc);

	static VariadicTy *create(Context &c, const Vector<Type *> &_args);

	inline void addArg(Type *ty)
	{
		args.push_back(ty);
	}
	inline Vector<Type *> &getArgs()
	{
		return args;
	}
	inline Type *getArg(const size_t &idx)
	{
		return args.size() > idx ? args[idx] : nullptr;
	}

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
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