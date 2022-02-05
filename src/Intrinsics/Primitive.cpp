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

#include "Intrinsics.hpp"
#include "ValueMgr.hpp"

#define GetInt(from) as<IntVal>(from->getValue())->getVal()
#define GetFlt(from) as<FltVal>(from->getValue())->getVal()

namespace sc
{
INTRINSIC(assn_int)
{
	args[0]->updateValue(c, args[1]->getValue());
	return stmt->updateValue(c, args[0]->getValue());
}

INTRINSIC(assn_flt)
{
	args[0]->updateValue(c, args[1]->getValue());
	return stmt->updateValue(c, args[0]->getValue());
}

INTRINSIC(add_int)
{
	auto res = GetInt(args[0]) + GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(sub_int)
{
	auto res = GetInt(args[0]) - GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(mul_int)
{
	auto res = GetInt(args[0]) * GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(div_int)
{
	auto res = GetInt(args[0]) / GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(mod_int)
{
	auto res = GetInt(args[0]) % GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(band_int)
{
	auto res = GetInt(args[0]) & GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(bor_int)
{
	auto res = GetInt(args[0]) | GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(bxor_int)
{
	auto res = GetInt(args[0]) ^ GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(lshift_int)
{
	auto res = GetInt(args[0]) << GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(rshift_int)
{
	auto res = GetInt(args[0]) >> GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(add_flt)
{
	auto res = GetFlt(args[0]) + GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(sub_flt)
{
	auto res = GetFlt(args[0]) - GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(mul_flt)
{
	auto res = GetFlt(args[0]) * GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(div_flt)
{
	auto res = GetFlt(args[0]) / GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(addassn_int)
{
	auto res = GetInt(args[0]) += GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(subassn_int)
{
	auto res = GetInt(args[0]) -= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(mulassn_int)
{
	auto res = GetInt(args[0]) *= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(divassn_int)
{
	auto res = GetInt(args[0]) /= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(modassn_int)
{
	auto res = GetInt(args[0]) %= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(bandassn_int)
{
	auto res = GetInt(args[0]) &= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(borassn_int)
{
	auto res = GetInt(args[0]) |= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(bxorassn_int)
{
	auto res = GetInt(args[0]) ^= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(lshiftassn_int)
{
	auto res = GetInt(args[0]) <<= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(rshiftassn_int)
{
	auto res = GetInt(args[0]) >>= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(addassn_flt)
{
	auto res = GetFlt(args[0]) += GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(subassn_flt)
{
	auto res = GetFlt(args[0]) -= GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(mulassn_flt)
{
	auto res = GetFlt(args[0]) *= GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(divassn_flt)
{
	auto res = GetFlt(args[0]) /= GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(logand_int)
{
	auto res = GetInt(args[0]) && GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(logor_int)
{
	auto res = GetInt(args[0]) || GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(eq_int)
{
	auto res = GetInt(args[0]) == GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(lt_int)
{
	auto res = GetInt(args[0]) < GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(gt_int)
{
	auto res = GetInt(args[0]) > GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(le_int)
{
	auto res = GetInt(args[0]) <= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(ge_int)
{
	auto res = GetInt(args[0]) >= GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(ne_int)
{
	auto res = GetInt(args[0]) != GetInt(args[1]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(logand_flt)
{
	auto res = GetFlt(args[0]) && GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(logor_flt)
{
	auto res = GetFlt(args[0]) || GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(eq_flt)
{
	auto res = GetFlt(args[0]) == GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(lt_flt)
{
	auto res = GetFlt(args[0]) < GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(gt_flt)
{
	auto res = GetFlt(args[0]) > GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(le_flt)
{
	auto res = GetFlt(args[0]) <= GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(ge_flt)
{
	auto res = GetFlt(args[0]) >= GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(ne_flt)
{
	auto res = GetFlt(args[0]) != GetFlt(args[1]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(uadd_int)
{
	auto res = +GetInt(args[0]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(usub_int)
{
	auto res = -GetInt(args[0]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(lognot_int)
{
	auto res = !GetInt(args[0]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(bnot_int)
{
	auto res = ~GetInt(args[0]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(incx_int)
{
	auto res = ++GetInt(args[0]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(decx_int)
{
	auto res = --GetInt(args[0]);
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(uadd_flt)
{
	auto res = +GetFlt(args[0]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(usub_flt)
{
	auto res = -GetFlt(args[0]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(lognot_flt)
{
	auto res = !GetFlt(args[0]);
	return stmt->updateValue(c, FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(xinc_int)
{
	auto res = GetInt(args[0])++;
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}

INTRINSIC(xdec_int)
{
	auto res = GetInt(args[0])--;
	return stmt->updateValue(c, IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
}
} // namespace sc