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

#define ADDFN(name, fn) vmgr.addVar(name, fn, nullptr, true)
#define ADDINTFN(name, fn) vmgr.addTypeFn(TINT, name, fn)
#define ADDFLTFN(name, fn) vmgr.addTypeFn(TFLT, name, fn)

void AddPrimitiveFuncs(Context &c, ValueManager &vmgr)
{
	TypeTy *g    = nullptr; // g = generic
	TypeTy *g2   = nullptr;
	IntTy *i1    = nullptr;
	IntTy *i32   = nullptr;
	IntTy *i32va = nullptr;
	IntTy *u64   = nullptr;
	Type *i8str  = nullptr;

	i8str = IntTy::create(c, 8, true);
	i8str->setConst();
	i8str = PtrTy::create(c, i8str, 0);
	i8str->setComptime();
	g = TypeTy::create(c);
	ADDFN("import", createFnVal(c, {i8str}, g, intrinsic_import, IPARSE));

	i1 = IntTy::create(c, 1, true);
	i1->setComptime();
	ADDFN("isMainSrc", createFnVal(c, {}, i1, intrinsic_ismainsrc, IPARSE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFN("isPrimitive", createFnVal(c, {g}, i1, intrinsic_isprimitive, IPARSE));

	g  = TypeTy::create(c);
	g2 = TypeTy::create(c);
	ADDFN("as", createFnVal(c, {g, g2}, g, intrinsic_as, IPARSE));

	g  = TypeTy::create(c);
	g2 = TypeTy::create(c);
	ADDFN("typeOf", createFnVal(c, {g}, g, intrinsic_typeof, IPARSE));

	g  = TypeTy::create(c);
	g2 = TypeTy::create(c);
	ADDFN("ptr", createFnVal(c, {g}, g2, intrinsic_ptr, IPARSE));

	g  = TypeTy::create(c);
	g2 = TypeTy::create(c);
	ADDFN("ref", createFnVal(c, {g}, g2, intrinsic_ptr, IPARSE));

	g   = TypeTy::create(c);
	u64 = IntTy::create(c, 64, false);
	ADDFN("sizeOf", createFnVal(c, {g}, u64, intrinsic_szof, IVALUE));

	i32 = IntTy::create(c, 32, true);
	i32->setComptime();
	ADDFN("valen", createFnVal(c, {}, i32, intrinsic_valen, IVALUE));

	g   = TypeTy::create(c);
	g2  = TypeTy::create(c);
	i32 = IntTy::create(c, 32, true);
	i32->setComptime();
	i32va = IntTy::create(c, 32, true);
	i32va->setVariadic();
	i32va->setComptime();
	ADDFN("array", createFnVal(c, {g, i32, i32va}, g2, intrinsic_array, IPARSE));

	g = TypeTy::create(c);
	ADDINTFN("__assn__", createFnVal(c, {g, g}, g, intrinsic_assn_int, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__assn__", createFnVal(c, {g, g}, g, intrinsic_assn_flt, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__add__", createFnVal(c, {g, g}, g, intrinsic_add_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__sub__", createFnVal(c, {g, g}, g, intrinsic_sub_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__mul__", createFnVal(c, {g, g}, g, intrinsic_mul_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__div__", createFnVal(c, {g, g}, g, intrinsic_div_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__mod__", createFnVal(c, {g, g}, g, intrinsic_mod_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__band__", createFnVal(c, {g, g}, g, intrinsic_band_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__bor__", createFnVal(c, {g, g}, g, intrinsic_bor_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__bxor__", createFnVal(c, {g, g}, g, intrinsic_bxor_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__lshift__", createFnVal(c, {g, g}, g, intrinsic_lshift_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__rshift__", createFnVal(c, {g, g}, g, intrinsic_rshift_int, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__add__", createFnVal(c, {g, g}, g, intrinsic_add_flt, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__sub__", createFnVal(c, {g, g}, g, intrinsic_sub_flt, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__mul__", createFnVal(c, {g, g}, g, intrinsic_mul_flt, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__div__", createFnVal(c, {g, g}, g, intrinsic_div_flt, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__add_assn__", createFnVal(c, {g, g}, g, intrinsic_addassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__sub_assn__", createFnVal(c, {g, g}, g, intrinsic_subassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__mul_assn__", createFnVal(c, {g, g}, g, intrinsic_mulassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__div_assn__", createFnVal(c, {g, g}, g, intrinsic_divassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__mod_assn__", createFnVal(c, {g, g}, g, intrinsic_modassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__band_assn__", createFnVal(c, {g, g}, g, intrinsic_bandassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__bor_assn__", createFnVal(c, {g, g}, g, intrinsic_borassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__bxor_assn__", createFnVal(c, {g, g}, g, intrinsic_bxorassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__lshift_assn__", createFnVal(c, {g, g}, g, intrinsic_lshiftassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__rshift_assn__", createFnVal(c, {g, g}, g, intrinsic_rshiftassn_int, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__add_assn__", createFnVal(c, {g, g}, g, intrinsic_addassn_flt, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__sub_assn__", createFnVal(c, {g, g}, g, intrinsic_subassn_flt, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__mul_assn__", createFnVal(c, {g, g}, g, intrinsic_mulassn_flt, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__div_assn__", createFnVal(c, {g, g}, g, intrinsic_divassn_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__logand__", createFnVal(c, {g, g}, i1, intrinsic_logand_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__logor__", createFnVal(c, {g, g}, i1, intrinsic_logor_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__eq__", createFnVal(c, {g, g}, i1, intrinsic_eq_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__lt__", createFnVal(c, {g, g}, i1, intrinsic_lt_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__gt__", createFnVal(c, {g, g}, i1, intrinsic_gt_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__le__", createFnVal(c, {g, g}, i1, intrinsic_le_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__ge__", createFnVal(c, {g, g}, i1, intrinsic_ge_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__ne__", createFnVal(c, {g, g}, i1, intrinsic_ne_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__logand__", createFnVal(c, {g, g}, i1, intrinsic_logand_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__logor__", createFnVal(c, {g, g}, i1, intrinsic_logor_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__eq__", createFnVal(c, {g, g}, i1, intrinsic_eq_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__lt__", createFnVal(c, {g, g}, i1, intrinsic_lt_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__gt__", createFnVal(c, {g, g}, i1, intrinsic_gt_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__le__", createFnVal(c, {g, g}, i1, intrinsic_le_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__ge__", createFnVal(c, {g, g}, i1, intrinsic_ge_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__ne__", createFnVal(c, {g, g}, i1, intrinsic_ne_flt, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__uadd__", createFnVal(c, {g}, g, intrinsic_uadd_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__usub__", createFnVal(c, {g}, g, intrinsic_usub_int, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDINTFN("__lognot__", createFnVal(c, {g}, i1, intrinsic_lognot_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__bnot__", createFnVal(c, {g}, g, intrinsic_bnot_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__incx__", createFnVal(c, {g}, g, intrinsic_incx_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__decx__", createFnVal(c, {g}, g, intrinsic_decx_int, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__uadd__", createFnVal(c, {g}, g, intrinsic_uadd_flt, IVALUE));

	g = TypeTy::create(c);
	ADDFLTFN("__usub__", createFnVal(c, {g}, g, intrinsic_usub_flt, IVALUE));

	i1 = IntTy::create(c, 1, true);
	g  = TypeTy::create(c);
	ADDFLTFN("__lognot__", createFnVal(c, {g}, i1, intrinsic_lognot_flt, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__xinc__", createFnVal(c, {g}, g, intrinsic_xinc_int, IVALUE));

	g = TypeTy::create(c);
	ADDINTFN("__xdec__", createFnVal(c, {g}, g, intrinsic_xdec_int, IVALUE));
}
} // namespace sc