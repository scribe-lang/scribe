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

#include "Context.hpp"

namespace sc
{
enum Types
{
	TVOID,
	// TNIL,  // = i1 (0)
	// TBOOL, // = i1
	TANY,

	TI1,
	TI8,
	TI16,
	TI32,
	TI64,

	TU8,
	TU16,
	TU32,
	TU64,

	TF32,
	TF64,

	TPTR,
	TFUNC,
	TSTRUCT,
	TENUM,
};

enum TypeInfoMask
{
	REF	 = 1 << 0, // is a reference
	STATIC	 = 1 << 1, // is static
	CONST	 = 1 << 2, // is const
	VOLATILE = 1 << 3, // is volatile
	VARIADIC = 1 << 4, // is variadic
};

class Type
{
	Types type;
	size_t info;

public:
	Type(const Types &type, const size_t &info);
	virtual ~Type();

#define IsTyX(Fn, Ty)                 \
	inline bool is##Fn() const    \
	{                             \
		return type == T##Ty; \
	}
	IsTyX(Void, VOID);
	IsTyX(Any, ANY);
	IsTyX(Int1, I1);
	IsTyX(Int8, I8);
	IsTyX(Int16, I16);
	IsTyX(Int32, I32);
	IsTyX(Int64, I64);
	IsTyX(UInt8, U8);
	IsTyX(UInt16, U16);
	IsTyX(UInt32, U32);
	IsTyX(UInt64, U64);
	IsTyX(Flt32, F32);
	IsTyX(Flt64, F64);
	IsTyX(Ptr, PTR);
	IsTyX(Func, FUNC);
	IsTyX(Struct, STRUCT);
	IsTyX(Enum, ENUM);

#define IsModifierX(Fn, Mod)       \
	inline bool is##Fn() const \
	{                          \
		return info & Mod; \
	}
	IsModifierX(Static, STATIC);
	IsModifierX(Const, CONST);
	IsModifierX(Volatile, VOLATILE);
	IsModifierX(Ref, REF);

#define SetModifierX(Fn, Mod) \
	inline void set##Fn() \
	{                     \
		info |= Mod;  \
	}
	SetModifierX(Static, STATIC);
	SetModifierX(Const, CONST);
	SetModifierX(Volatile, VOLATILE);
	SetModifierX(Ref, REF);
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
		~Ty();                         \
		static Ty *create(Context &c); \
	}

BasicTypeDecl(VoidTy);
BasicTypeDecl(AnyTy);
BasicTypeDecl(Int1Ty);
BasicTypeDecl(Int8Ty);
BasicTypeDecl(Int16Ty);
BasicTypeDecl(Int32Ty);
BasicTypeDecl(Int64Ty);
BasicTypeDecl(UInt8Ty);
BasicTypeDecl(UInt16Ty);
BasicTypeDecl(UInt32Ty);
BasicTypeDecl(UInt64Ty);
BasicTypeDecl(Flt32Ty);
BasicTypeDecl(Flt64Ty);

class PtrTy : public Type
{
	Type *to;

public:
	PtrTy(Type *to);
	~PtrTy();

	static PtrTy *create(Context &c, Type *ptr_to);
};
} // namespace sc

#endif // TYPES_HPP