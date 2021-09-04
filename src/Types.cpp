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

#include "Types.hpp"

namespace sc
{
Type::Type(const Types &type, const size_t &info) : type(type), info(info) {}
Type::~Type() {}

#define BasicTypeDefine(Ty, EnumName)     \
	Ty::Ty() : Type(EnumName, 0) {}   \
	Ty::~Ty() {}                      \
	Ty *Ty::create(Context &c)        \
	{                                 \
		return c.allocType<Ty>(); \
	}

BasicTypeDefine(VoidTy, TVOID);
BasicTypeDefine(AnyTy, TANY);
BasicTypeDefine(Int1Ty, TI1);
BasicTypeDefine(Int8Ty, TI8);
BasicTypeDefine(Int16Ty, TI16);
BasicTypeDefine(Int32Ty, TI32);
BasicTypeDefine(Int64Ty, TI64);
BasicTypeDefine(UInt8Ty, TU8);
BasicTypeDefine(UInt16Ty, TU16);
BasicTypeDefine(UInt32Ty, TU32);
BasicTypeDefine(UInt64Ty, TU64);
BasicTypeDefine(Flt32Ty, TF32);
BasicTypeDefine(Flt64Ty, TF64);

PtrTy::PtrTy(Type *to) : Type(TPTR, 0), to(to) {}
PtrTy::~PtrTy() {}

PtrTy *PtrTy::create(Context &c, Type *ptr_to)
{
	return c.allocType<PtrTy>(ptr_to);
}
} // namespace sc
