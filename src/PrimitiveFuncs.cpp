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

#include "PrimitiveTypeFuncs.hpp"

namespace sc
{
uint64_t createFnVal(Context &c, const std::vector<Type *> &args, Type *ret, IntrinsicFn fn,
		     const IntrinType &inty)
{
	FuncTy *t = FuncTy::create(c, nullptr, args, ret, fn, inty, false);
	return createValueIDWith(FuncVal::create(c, t));
}

void addIntFn(Context &c, ValueManager &vmgr, const std::string &name, const uint64_t &fid)
{
	static std::vector<int> bits  = {1, 8, 16, 32, 64};
	static std::vector<bool> sign = {true, false};
	static std::unordered_map<int, IntTy *> tys;

	for(auto s : sign) {
		for(auto &b : bits) {
			IntTy *ty = nullptr;
			if(tys.find(b + s) == tys.end()) {
				tys[b + s] = IntTy::create(c, b, s);
			}
			ty = tys[b + s];
			vmgr.addTypeFn(ty->getID(), name, fid);
		}
	}
}

void addFltFn(Context &c, ValueManager &vmgr, const std::string &name, const uint64_t &fid)
{
	static std::vector<int> bits = {32, 64};
	static std::unordered_map<int, FltTy *> tys;

	for(auto &b : bits) {
		FltTy *ty = nullptr;
		if(tys.find(b) == tys.end()) {
			tys[b] = FltTy::create(c, b);
		}
		ty = tys[b];
		vmgr.addTypeFn(ty->getID(), name, fid);
	}
}

#define ADDFN(name, fn) vmgr.addVar(name, fn, nullptr, true)
#define ADDINTFN(name, fn) addIntFn(c, vmgr, name, fn)
#define ADDFLTFN(name, fn) addFltFn(c, vmgr, name, fn)
#define ADDPTRFN(name, fn) vmgr.addTypeFn(TPTR, name, fn)

void AddPrimitiveFuncs(Context &c, ValueManager &vmgr)
{
	AnyTy *a     = nullptr;
	TypeTy *g    = nullptr; // g = generic
	TypeTy *g2   = nullptr;
	IntTy *i0    = nullptr; // 0 bits => any int
	FltTy *f0    = nullptr; // 0 bits => any flt
	IntTy *i1    = nullptr;
	IntTy *i32   = nullptr;
	IntTy *i32va = nullptr;
	IntTy *u64   = nullptr;
	Type *i8str  = nullptr;
	VoidTy *v    = nullptr;

	i8str = mkStrTy(c);
	i8str->setComptime();
	g = mkTypeTy(c);
	ADDFN("import", createFnVal(c, {i8str}, g, intrinsic_import, IPARSE));

	i1 = mkI1Ty(c);
	i1->setComptime();
	ADDFN("isMainSrc", createFnVal(c, {}, i1, intrinsic_ismainsrc, IPARSE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	ADDFN("isPrimitive", createFnVal(c, {g}, i1, intrinsic_isprimitive, IPARSE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	ADDFN("isPrimitiveOrPtr", createFnVal(c, {g}, i1, intrinsic_isprimitiveorptr, IPARSE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	ADDFN("isCString", createFnVal(c, {g}, i1, intrinsic_iscstring, IPARSE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	ADDFN("isCChar", createFnVal(c, {g}, i1, intrinsic_iscchar, IPARSE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	g2 = mkTypeTy(c);
	ADDFN("isEqualTy", createFnVal(c, {g, g2}, i1, intrinsic_isequalty, IPARSE));

	g  = mkTypeTy(c);
	g2 = mkTypeTy(c);
	ADDFN("as", createFnVal(c, {g, g2}, g, intrinsic_as, IPARSE));

	g  = mkTypeTy(c);
	g2 = mkTypeTy(c);
	ADDFN("typeOf", createFnVal(c, {g}, g, intrinsic_typeof, IPARSE));

	g  = mkTypeTy(c);
	g2 = mkTypeTy(c);
	ADDFN("ptr", createFnVal(c, {g}, g2, intrinsic_ptr, IPARSE));

	g  = mkTypeTy(c);
	g2 = mkTypeTy(c);
	ADDFN("ref", createFnVal(c, {g}, g2, intrinsic_ptr, IPARSE));

	g   = mkTypeTy(c);
	u64 = IntTy::create(c, 64, false);
	ADDFN("sizeOf", createFnVal(c, {g}, u64, intrinsic_szof, IVALUE));

	i32 = mkI32Ty(c);
	i32->setComptime();
	ADDFN("valen", createFnVal(c, {}, i32, intrinsic_valen, IVALUE));

	a = mkAnyTy(c);
	a->setVariadic();
	v = mkVoidTy(c);
	ADDFN("compileError", createFnVal(c, {a}, v, intrinsic_compileerror, IPARSE));

	g   = mkTypeTy(c);
	g2  = mkTypeTy(c);
	i32 = mkI32Ty(c);
	i32->setComptime();
	i32va = mkI32Ty(c);
	i32va->setVariadic();
	i32va->setComptime();
	ADDFN("array", createFnVal(c, {g, i32, i32va}, g2, intrinsic_array, IPARSE));

	g = mkTypeTy(c);
	ADDPTRFN("__assn__", createFnVal(c, {g, g}, g, intrinsic_assn_ptr, IVALUE));

	///////////////////////////////////////////////////////////////////////////////////////////
	// Generated Arithmetic Primitives
	///////////////////////////////////////////////////////////////////////////////////////////
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__assn__", createFnVal(c, {g, i0}, g, intrinsic_assn_int, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__assn__", createFnVal(c, {g, f0}, g, intrinsic_assn_flt, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__add__", createFnVal(c, {g, i0}, g, intrinsic_add_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__sub__", createFnVal(c, {g, i0}, g, intrinsic_sub_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__mul__", createFnVal(c, {g, i0}, g, intrinsic_mul_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__div__", createFnVal(c, {g, i0}, g, intrinsic_div_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__mod__", createFnVal(c, {g, i0}, g, intrinsic_mod_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__band__", createFnVal(c, {g, i0}, g, intrinsic_band_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__bor__", createFnVal(c, {g, i0}, g, intrinsic_bor_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__bxor__", createFnVal(c, {g, i0}, g, intrinsic_bxor_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__lshift__", createFnVal(c, {g, i0}, g, intrinsic_lshift_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__rshift__", createFnVal(c, {g, i0}, g, intrinsic_rshift_int, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__add__", createFnVal(c, {g, f0}, g, intrinsic_add_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__sub__", createFnVal(c, {g, f0}, g, intrinsic_sub_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__mul__", createFnVal(c, {g, f0}, g, intrinsic_mul_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__div__", createFnVal(c, {g, f0}, g, intrinsic_div_flt, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__add_assn__", createFnVal(c, {g, i0}, g, intrinsic_addassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__sub_assn__", createFnVal(c, {g, i0}, g, intrinsic_subassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__mul_assn__", createFnVal(c, {g, i0}, g, intrinsic_mulassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__div_assn__", createFnVal(c, {g, i0}, g, intrinsic_divassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__mod_assn__", createFnVal(c, {g, i0}, g, intrinsic_modassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__band_assn__", createFnVal(c, {g, i0}, g, intrinsic_bandassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__bor_assn__", createFnVal(c, {g, i0}, g, intrinsic_borassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__bxor_assn__", createFnVal(c, {g, i0}, g, intrinsic_bxorassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__lshift_assn__", createFnVal(c, {g, i0}, g, intrinsic_lshiftassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__rshift_assn__", createFnVal(c, {g, i0}, g, intrinsic_rshiftassn_int, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__add_assn__", createFnVal(c, {g, f0}, g, intrinsic_addassn_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__sub_assn__", createFnVal(c, {g, f0}, g, intrinsic_subassn_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__mul_assn__", createFnVal(c, {g, f0}, g, intrinsic_mulassn_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__div_assn__", createFnVal(c, {g, f0}, g, intrinsic_divassn_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__logand__", createFnVal(c, {g, i0}, i1, intrinsic_logand_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__logor__", createFnVal(c, {g, i0}, i1, intrinsic_logor_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__eq__", createFnVal(c, {g, i0}, i1, intrinsic_eq_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__lt__", createFnVal(c, {g, i0}, i1, intrinsic_lt_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__gt__", createFnVal(c, {g, i0}, i1, intrinsic_gt_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__le__", createFnVal(c, {g, i0}, i1, intrinsic_le_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__ge__", createFnVal(c, {g, i0}, i1, intrinsic_ge_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	i0->setRef();
	i0->setConst();
	ADDINTFN("__ne__", createFnVal(c, {g, i0}, i1, intrinsic_ne_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__logand__", createFnVal(c, {g, f0}, i1, intrinsic_logand_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__logor__", createFnVal(c, {g, f0}, i1, intrinsic_logor_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__eq__", createFnVal(c, {g, f0}, i1, intrinsic_eq_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__lt__", createFnVal(c, {g, f0}, i1, intrinsic_lt_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__gt__", createFnVal(c, {g, f0}, i1, intrinsic_gt_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__le__", createFnVal(c, {g, f0}, i1, intrinsic_le_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__ge__", createFnVal(c, {g, f0}, i1, intrinsic_ge_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	f0->setRef();
	f0->setConst();
	ADDFLTFN("__ne__", createFnVal(c, {g, f0}, i1, intrinsic_ne_flt, IVALUE));

	g = mkTypeTy(c);
	ADDINTFN("__uadd__", createFnVal(c, {g}, g, intrinsic_uadd_int, IVALUE));

	g = mkTypeTy(c);
	ADDINTFN("__usub__", createFnVal(c, {g}, g, intrinsic_usub_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	ADDINTFN("__lognot__", createFnVal(c, {g}, i1, intrinsic_lognot_int, IVALUE));

	g = mkTypeTy(c);
	ADDINTFN("__bnot__", createFnVal(c, {g}, g, intrinsic_bnot_int, IVALUE));

	g = mkTypeTy(c);
	ADDINTFN("__incx__", createFnVal(c, {g}, g, intrinsic_incx_int, IVALUE));

	g = mkTypeTy(c);
	ADDINTFN("__decx__", createFnVal(c, {g}, g, intrinsic_decx_int, IVALUE));

	g = mkTypeTy(c);
	ADDFLTFN("__uadd__", createFnVal(c, {g}, g, intrinsic_uadd_flt, IVALUE));

	g = mkTypeTy(c);
	ADDFLTFN("__usub__", createFnVal(c, {g}, g, intrinsic_usub_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	ADDFLTFN("__lognot__", createFnVal(c, {g}, i1, intrinsic_lognot_flt, IVALUE));

	g = mkTypeTy(c);
	ADDINTFN("__xinc__", createFnVal(c, {g}, g, intrinsic_xinc_int, IVALUE));

	g = mkTypeTy(c);
	ADDINTFN("__xdec__", createFnVal(c, {g}, g, intrinsic_xdec_int, IVALUE));
}
} // namespace sc