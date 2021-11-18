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

#include "Intrinsics.hpp"
#include "ValueMgr.hpp"

#define GetInt(from) as<IntVal>(from->getValue())->getVal()
#define GetFlt(from) as<FltVal>(from->getValue())->getVal()

namespace sc
{
INTRINSIC(assn_int)
{
	args[0]->updateValue(args[1]->getValue());
	stmt->updateValue(args[0]->getValue());
	return true;
}

INTRINSIC(assn_flt)
{
	args[0]->updateValue(args[1]->getValue());
	stmt->updateValue(args[0]->getValue());
	return true;
}

INTRINSIC(add_int)
{
	auto res = GetInt(args[0]) + GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(sub_int)
{
	auto res = GetInt(args[0]) - GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(mul_int)
{
	auto res = GetInt(args[0]) * GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(div_int)
{
	auto res = GetInt(args[0]) / GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(mod_int)
{
	auto res = GetInt(args[0]) % GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(band_int)
{
	auto res = GetInt(args[0]) & GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(bor_int)
{
	auto res = GetInt(args[0]) | GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(bxor_int)
{
	auto res = GetInt(args[0]) ^ GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(lshift_int)
{
	auto res = GetInt(args[0]) << GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(rshift_int)
{
	auto res = GetInt(args[0]) >> GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(add_flt)
{
	auto res = GetFlt(args[0]) + GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(sub_flt)
{
	auto res = GetFlt(args[0]) - GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(mul_flt)
{
	auto res = GetFlt(args[0]) * GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(div_flt)
{
	auto res = GetFlt(args[0]) / GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(addassn_int)
{
	auto res = GetInt(args[0]) += GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(subassn_int)
{
	auto res = GetInt(args[0]) -= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(mulassn_int)
{
	auto res = GetInt(args[0]) *= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(divassn_int)
{
	auto res = GetInt(args[0]) /= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(modassn_int)
{
	auto res = GetInt(args[0]) %= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(bandassn_int)
{
	auto res = GetInt(args[0]) &= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(borassn_int)
{
	auto res = GetInt(args[0]) |= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(bxorassn_int)
{
	auto res = GetInt(args[0]) ^= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(lshiftassn_int)
{
	auto res = GetInt(args[0]) <<= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(rshiftassn_int)
{
	auto res = GetInt(args[0]) >>= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(addassn_flt)
{
	auto res = GetFlt(args[0]) += GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(subassn_flt)
{
	auto res = GetFlt(args[0]) -= GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(mulassn_flt)
{
	auto res = GetFlt(args[0]) *= GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(divassn_flt)
{
	auto res = GetFlt(args[0]) /= GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(logand_int)
{
	auto res = GetInt(args[0]) && GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(logor_int)
{
	auto res = GetInt(args[0]) || GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(eq_int)
{
	auto res = GetInt(args[0]) == GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(lt_int)
{
	auto res = GetInt(args[0]) < GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(gt_int)
{
	auto res = GetInt(args[0]) > GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(le_int)
{
	auto res = GetInt(args[0]) <= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(ge_int)
{
	auto res = GetInt(args[0]) >= GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(ne_int)
{
	auto res = GetInt(args[0]) != GetInt(args[1]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(logand_flt)
{
	auto res = GetFlt(args[0]) && GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(logor_flt)
{
	auto res = GetFlt(args[0]) || GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(eq_flt)
{
	auto res = GetFlt(args[0]) == GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(lt_flt)
{
	auto res = GetFlt(args[0]) < GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(gt_flt)
{
	auto res = GetFlt(args[0]) > GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(le_flt)
{
	auto res = GetFlt(args[0]) <= GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(ge_flt)
{
	auto res = GetFlt(args[0]) >= GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(ne_flt)
{
	auto res = GetFlt(args[0]) != GetFlt(args[1]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(uadd_int)
{
	auto res = +GetInt(args[0]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(usub_int)
{
	auto res = -GetInt(args[0]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(lognot_int)
{
	auto res = !GetInt(args[0]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(bnot_int)
{
	auto res = ~GetInt(args[0]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(incx_int)
{
	auto res = ++GetInt(args[0]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(decx_int)
{
	auto res = --GetInt(args[0]);
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(uadd_flt)
{
	auto res = +GetFlt(args[0]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(usub_flt)
{
	auto res = -GetFlt(args[0]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(lognot_flt)
{
	auto res = !GetFlt(args[0]);
	stmt->updateValue(FltVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(xinc_int)
{
	auto res = GetInt(args[0])++;
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}

INTRINSIC(xdec_int)
{
	auto res = GetInt(args[0])--;
	stmt->updateValue(IntVal::create(c, stmt->getValueTy(), CDTRUE, res));
	return true;
}
} // namespace sc