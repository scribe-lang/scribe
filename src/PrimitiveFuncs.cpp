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

#include "PrimitiveTypeFuncs.hpp"

namespace sc
{
uint64_t createComptimeFnVal(Context &c, const Vector<Type *> &args, Type *ret,
			     const Vector<bool> &argcomptime, IntrinsicFn fn,
			     const IntrinType &inty, bool is_va = false)
{
	FuncTy *t = FuncTy::create(c, nullptr, args, ret, argcomptime, fn, inty, false, is_va);
	return createValueIDWith(FuncVal::create(c, t));
}
inline uint64_t createFnVal(Context &c, const Vector<Type *> &args, Type *ret, IntrinsicFn fn,
			    const IntrinType &inty, bool is_va = false)
{
	return createComptimeFnVal(c, args, ret, {}, fn, inty, is_va);
}

void addIntFn(Context &c, ValueManager &vmgr, StringRef name, const uint64_t &fid)
{
	static Vector<int> bits	 = {1, 8, 16, 32, 64};
	static Vector<bool> sign = {true, false};
	static Map<int, IntTy *> tys;

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

void addFltFn(Context &c, ValueManager &vmgr, StringRef name, const uint64_t &fid)
{
	static Vector<int> bits = {32, 64};
	static Map<int, FltTy *> tys;

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
	g     = mkTypeTy(c);
	ADDFN("import", createComptimeFnVal(c, {i8str}, g, {true}, intrinsic_import, IPARSE));

	i1 = mkI1Ty(c);
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
	ADDFN("ref", createFnVal(c, {g}, g2, intrinsic_ref, IPARSE));

	g  = mkTypeTy(c);
	g2 = mkTypeTy(c);
	ADDFN("cons", createFnVal(c, {g}, g2, intrinsic_cons, IPARSE));

	g   = mkTypeTy(c);
	u64 = mkU64Ty(c);
	ADDFN("sizeOf", createFnVal(c, {g}, u64, intrinsic_szof, IVALUE));

	// valen must be IPARSE since the value has to be decided during type assignment
	// ie, cannot be decided during value assignment pass
	i32 = mkI32Ty(c);
	ADDFN("valen", createFnVal(c, {}, i32, intrinsic_valen, IPARSE));

	i32 = mkI32Ty(c);
	ADDFN("getOSID", createFnVal(c, {}, i32, intrinsic_getosid, IVALUE));

	i32 = mkI32Ty(c);
	ADDFN("sysPathMax", createFnVal(c, {}, i32, intrinsic_syspathmax, IVALUE));

	a = mkAnyTy(c);
	v = mkVoidTy(c);
	ADDFN("compileError", createFnVal(c, {a}, v, intrinsic_compileerror, IPARSE, true));

	g     = mkTypeTy(c);
	g2    = mkTypeTy(c);
	i32   = mkI32Ty(c);
	i32va = mkI32Ty(c);
	ADDFN("array", createComptimeFnVal(c, {g, i32, i32va}, g2, {false, true, true},
					   intrinsic_array, IPARSE, true));

	g = mkTypeTy(c);
	ADDPTRFN("__assn__", createFnVal(c, {g, g}, g, intrinsic_assn_ptr, IVALUE));

	///////////////////////////////////////////////////////////////////////////////////////////
	// Generated Arithmetic Primitives
	///////////////////////////////////////////////////////////////////////////////////////////
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__assn__", createFnVal(c, {g, i0}, g, intrinsic_assn_int, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__assn__", createFnVal(c, {g, f0}, g, intrinsic_assn_flt, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__add__", createFnVal(c, {g, i0}, g, intrinsic_add_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__sub__", createFnVal(c, {g, i0}, g, intrinsic_sub_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__mul__", createFnVal(c, {g, i0}, g, intrinsic_mul_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__div__", createFnVal(c, {g, i0}, g, intrinsic_div_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__mod__", createFnVal(c, {g, i0}, g, intrinsic_mod_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__band__", createFnVal(c, {g, i0}, g, intrinsic_band_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__bor__", createFnVal(c, {g, i0}, g, intrinsic_bor_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__bxor__", createFnVal(c, {g, i0}, g, intrinsic_bxor_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__lshift__", createFnVal(c, {g, i0}, g, intrinsic_lshift_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__rshift__", createFnVal(c, {g, i0}, g, intrinsic_rshift_int, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__add__", createFnVal(c, {g, f0}, g, intrinsic_add_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__sub__", createFnVal(c, {g, f0}, g, intrinsic_sub_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__mul__", createFnVal(c, {g, f0}, g, intrinsic_mul_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__div__", createFnVal(c, {g, f0}, g, intrinsic_div_flt, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__add_assn__", createFnVal(c, {g, i0}, g, intrinsic_addassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__sub_assn__", createFnVal(c, {g, i0}, g, intrinsic_subassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__mul_assn__", createFnVal(c, {g, i0}, g, intrinsic_mulassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__div_assn__", createFnVal(c, {g, i0}, g, intrinsic_divassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__mod_assn__", createFnVal(c, {g, i0}, g, intrinsic_modassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__band_assn__", createFnVal(c, {g, i0}, g, intrinsic_bandassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__bor_assn__", createFnVal(c, {g, i0}, g, intrinsic_borassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__bxor_assn__", createFnVal(c, {g, i0}, g, intrinsic_bxorassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__lshift_assn__", createFnVal(c, {g, i0}, g, intrinsic_lshiftassn_int, IVALUE));

	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__rshift_assn__", createFnVal(c, {g, i0}, g, intrinsic_rshiftassn_int, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__add_assn__", createFnVal(c, {g, f0}, g, intrinsic_addassn_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__sub_assn__", createFnVal(c, {g, f0}, g, intrinsic_subassn_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__mul_assn__", createFnVal(c, {g, f0}, g, intrinsic_mulassn_flt, IVALUE));

	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__div_assn__", createFnVal(c, {g, f0}, g, intrinsic_divassn_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__logand__", createFnVal(c, {g, i0}, i1, intrinsic_logand_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__logor__", createFnVal(c, {g, i0}, i1, intrinsic_logor_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__eq__", createFnVal(c, {g, i0}, i1, intrinsic_eq_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__lt__", createFnVal(c, {g, i0}, i1, intrinsic_lt_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__gt__", createFnVal(c, {g, i0}, i1, intrinsic_gt_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__le__", createFnVal(c, {g, i0}, i1, intrinsic_le_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__ge__", createFnVal(c, {g, i0}, i1, intrinsic_ge_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	i0 = mkI0Ty(c);
	ADDINTFN("__ne__", createFnVal(c, {g, i0}, i1, intrinsic_ne_int, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__logand__", createFnVal(c, {g, f0}, i1, intrinsic_logand_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__logor__", createFnVal(c, {g, f0}, i1, intrinsic_logor_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__eq__", createFnVal(c, {g, f0}, i1, intrinsic_eq_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__lt__", createFnVal(c, {g, f0}, i1, intrinsic_lt_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__gt__", createFnVal(c, {g, f0}, i1, intrinsic_gt_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__le__", createFnVal(c, {g, f0}, i1, intrinsic_le_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
	ADDFLTFN("__ge__", createFnVal(c, {g, f0}, i1, intrinsic_ge_flt, IVALUE));

	i1 = mkI1Ty(c);
	g  = mkTypeTy(c);
	f0 = mkF0Ty(c);
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