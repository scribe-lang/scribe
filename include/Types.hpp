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

class Stmt;
class StmtExpr;
class StmtVar;
class StmtStruct;
class StmtFnDef;
class StmtFnSig;
class StmtFnCallInfo;
typedef bool (*IntrinsicFn)(Context &c, StmtExpr *stmt, Stmt **source, Vector<Stmt *> &args);
#define INTRINSIC(name) \
	bool intrinsic_##name(Context &c, StmtExpr *stmt, Stmt **source, Vector<Stmt *> &args)

class Type
{
	Types type;

public:
	Type(const Types &type);
	virtual ~Type();

	bool isBaseCompatible(Context &c, Type *rhs, const ModuleLoc *loc);

	String baseToStr();
	bool requiresCast(Type *other);

	virtual uint32_t getUniqID(); // used by codegen
	virtual uint32_t getID();
	virtual bool isTemplate(const size_t &weak_depth = 0);
	virtual String toStr(const size_t &weak_depth = 0);
	virtual bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	virtual void unmergeTemplates(const size_t &weak_depth = 0);
	virtual bool isCompatible(Context &c, Type *rhs, const ModuleLoc *loc);
	virtual Type *specialize(Context &c, const size_t &weak_depth = 0) = 0;

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

	inline uint32_t getBaseID() const
	{
		return type;
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
	~VoidTy();

	String toStr(const size_t &weak_depth = 0);

	static VoidTy *get(Context &c);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};
class AnyTy : public Type
{
public:
	AnyTy();
	~AnyTy();

	String toStr(const size_t &weak_depth = 0);

	static AnyTy *get(Context &c);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

	Value *toDefaultValue(Context &c, const ModuleLoc *loc, ContainsData cd,
			      const size_t &weak_depth = 0);
};

class IntTy : public Type
{
	uint16_t bits;
	bool sign; // signed

public:
	IntTy(uint16_t bits, bool sign);
	~IntTy();

	uint32_t getID();
	String toStr(const size_t &weak_depth = 0);

	static IntTy *get(Context &c, uint16_t _bits, bool _sign);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

	inline uint16_t getBits() const
	{
		return bits;
	}
	inline bool isSigned() const
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
	FltTy(uint16_t bits);
	~FltTy();

	uint32_t getID();
	String toStr(const size_t &weak_depth = 0);

	static FltTy *get(Context &c, uint16_t _bits);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

	inline uint16_t getBits() const
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
	TypeTy(uint32_t containedtyid);
	~TypeTy();

	uint32_t getUniqID();
	uint32_t getID();
	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);

	static TypeTy *get(Context &c);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

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
	PtrTy(Type *to, uint16_t count, bool is_weak);
	~PtrTy();

	uint32_t getUniqID();
	uint32_t getID();
	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);

	static PtrTy *get(Context &c, Type *ptr_to, uint16_t count, bool is_weak);
	static PtrTy *getStr(Context &c);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

	inline void setWeak(bool weak)
	{
		is_weak = weak;
	}
	inline Type *getTo()
	{
		return to;
	}
	inline uint16_t getCount()
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
	uint32_t id; // to differentiate between each struct ty
	StmtStruct *decl;
	Map<StringRef, size_t> fieldpos;
	Vector<StringRef> fieldnames;
	Vector<Type *> fields;
	Map<StringRef, size_t> templatepos;
	Vector<StringRef> templatenames;
	Vector<TypeTy *> templates;
	bool has_template;
	bool externed;

public:
	StructTy(StmtStruct *decl, const Vector<StringRef> &fieldnames,
		 const Vector<Type *> &fields, const Vector<StringRef> &templatenames,
		 const Vector<TypeTy *> &templates, bool externed);
	StructTy(uint32_t id, StmtStruct *decl, const Vector<StringRef> &fieldnames,
		 const Map<StringRef, size_t> &fieldpos, const Vector<Type *> &fields,
		 const Vector<StringRef> &templatenames, const Map<StringRef, size_t> &templatepos,
		 const Vector<TypeTy *> &templates, bool has_template, bool externed);
	~StructTy();

	uint32_t getUniqID();
	uint32_t getID();
	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, const ModuleLoc *loc);
	// specializes a structure type
	StructTy *applyTemplates(Context &c, const ModuleLoc *loc, const Vector<Type *> &actuals);
	// returns a NON-def struct type
	StructTy *instantiate(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &callargs);

	static StructTy *get(Context &c, StmtStruct *decl, const Vector<StringRef> &_fieldnames,
			     const Vector<Type *> &_fields, const Vector<StringRef> &_templatenames,
			     const Vector<TypeTy *> &_templates, bool _externed);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

	inline void insertField(StringRef name, Type *ty)
	{
		fieldpos[name] = fields.size();
		fieldnames.push_back(name);
		fields.push_back(ty);
	}
	inline void setExterned(bool ext)
	{
		externed = ext;
	}
	inline StmtStruct *getDecl()
	{
		return decl;
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
	inline void setTemplate(bool has_templ)
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
	uint32_t id; // to differentiate between each func ty
	StmtVar *var;
	StmtFnSig *sig;
	Vector<Type *> args;
	Type *ret;
	Vector<bool> argcomptime;
	IntrinsicFn intrin;
	IntrinType inty;
	uint32_t uniqid;
	bool externed;
	bool variadic;

public:
	FuncTy(StmtVar *var, const Vector<Type *> &args, Type *ret,
	       const Vector<bool> &_argcomptime, IntrinsicFn intrin, const IntrinType &inty,
	       bool externed, bool variadic);
	FuncTy(uint32_t id, StmtVar *var, StmtFnSig *sig, const Vector<Type *> &args, Type *ret,
	       const Vector<bool> &_argcomptime, IntrinsicFn intrin, const IntrinType &inty,
	       uint32_t uniqid, bool externed, bool variadic);
	~FuncTy();

	// returns ID of parameters + ret type
	uint32_t getSignatureID();
	uint32_t getNonUniqID();
	uint32_t getID();
	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, const ModuleLoc *loc);
	// specializes a function type using StmtFnCallInfo
	FuncTy *createCall(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &callargs);

	static FuncTy *get(Context &c, StmtVar *_var, const Vector<Type *> &_args, Type *_ret,
			   const Vector<bool> &_argcomptime, IntrinsicFn _intrin,
			   const IntrinType &_inty, bool _externed, bool _variadic);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

	// used in initTemplateFunc
	inline void setFuncID(FuncTy *other)
	{
		id     = other->id;
		uniqid = other->uniqid;
	}
	inline void setVar(StmtVar *v)
	{
		var = v;
		setSigFromVar();
	}
	inline void setSig(StmtFnSig *s)
	{
		sig = s;
	}
	void setSigFromVar();
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
	inline void setExterned(bool ext)
	{
		externed = ext;
	}
	inline void setVariadic(bool va)
	{
		variadic = va;
	}
	inline StmtVar *&getVar()
	{
		return var;
	}
	inline StmtFnSig *getSig()
	{
		return sig;
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
	inline bool isArgComptime(size_t idx)
	{
		return args.size() > idx ? argcomptime[idx] : false;
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
	inline bool isVariadic()
	{
		return variadic;
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
	~VariadicTy();

	bool isTemplate(const size_t &weak_depth = 0);
	String toStr(const size_t &weak_depth = 0);
	bool mergeTemplatesFrom(Type *ty, const size_t &weak_depth = 0);
	void unmergeTemplates(const size_t &weak_depth = 0);
	bool isCompatible(Context &c, Type *rhs, const ModuleLoc *loc);

	static VariadicTy *get(Context &c, const Vector<Type *> &_args);
	Type *specialize(Context &c, const size_t &weak_depth = 0);

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
} // namespace sc

#endif // TYPES_HPP