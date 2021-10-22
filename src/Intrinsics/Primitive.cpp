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
#include "TypeMgr.hpp"

#define GetType(i) args[i]->getType()

#define GetIntVal(i) as<IntVal>(args[i]->getValue())->getVal()
#define CreateIntVal(v) IntVal::create(c, v)
#define GetFltVal(i) as<FltVal>(args[i]->getValue())->getVal()
#define CreateFltVal(v) FltVal::create(c, v)

namespace sc
{
INTRINSIC(assn_int)
{
	args[0]->setVal(CreateIntVal(GetIntVal(1)));
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(assn_flt)
{
	args[0]->setVal(CreateFltVal(GetFltVal(1)));
	stmt->setVal(CreateFltVal(GetFltVal(0)));
	return true;
}

INTRINSIC(add_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) + GetIntVal(1)));
	return true;
}

INTRINSIC(sub_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) - GetIntVal(1)));
	return true;
}

INTRINSIC(mul_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) * GetIntVal(1)));
	return true;
}

INTRINSIC(div_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) / GetIntVal(1)));
	return true;
}

INTRINSIC(mod_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) % GetIntVal(1)));
	return true;
}

INTRINSIC(band_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) & GetIntVal(1)));
	return true;
}

INTRINSIC(bor_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) | GetIntVal(1)));
	return true;
}

INTRINSIC(bxor_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) ^ GetIntVal(1)));
	return true;
}

INTRINSIC(lshift_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) << GetIntVal(1)));
	return true;
}

INTRINSIC(rshift_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) >> GetIntVal(1)));
	return true;
}

INTRINSIC(add_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) + GetFltVal(1)));
	return true;
}

INTRINSIC(sub_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) - GetFltVal(1)));
	return true;
}

INTRINSIC(mul_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) * GetFltVal(1)));
	return true;
}

INTRINSIC(div_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) / GetFltVal(1)));
	return true;
}

INTRINSIC(addassn_int)
{
	GetIntVal(0) += GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(subassn_int)
{
	GetIntVal(0) -= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(mulassn_int)
{
	GetIntVal(0) *= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(divassn_int)
{
	GetIntVal(0) /= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(modassn_int)
{
	GetIntVal(0) %= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(bandassn_int)
{
	GetIntVal(0) &= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(borassn_int)
{
	GetIntVal(0) |= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(bxorassn_int)
{
	GetIntVal(0) ^= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(lshiftassn_int)
{
	GetIntVal(0) <<= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(rshiftassn_int)
{
	GetIntVal(0) >>= GetIntVal(1);
	stmt->setVal(CreateIntVal(GetIntVal(0)));
	return true;
}

INTRINSIC(addassn_flt)
{
	GetFltVal(0) += GetFltVal(1);
	stmt->setVal(CreateFltVal(GetFltVal(0)));
	return true;
}

INTRINSIC(subassn_flt)
{
	GetFltVal(0) -= GetFltVal(1);
	stmt->setVal(CreateFltVal(GetFltVal(0)));
	return true;
}

INTRINSIC(mulassn_flt)
{
	GetFltVal(0) *= GetFltVal(1);
	stmt->setVal(CreateFltVal(GetFltVal(0)));
	return true;
}

INTRINSIC(divassn_flt)
{
	GetFltVal(0) /= GetFltVal(1);
	stmt->setVal(CreateFltVal(GetFltVal(0)));
	return true;
}

INTRINSIC(logand_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) && GetIntVal(1)));
	return true;
}

INTRINSIC(logor_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) || GetIntVal(1)));
	return true;
}

INTRINSIC(eq_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) == GetIntVal(1)));
	return true;
}

INTRINSIC(lt_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) < GetIntVal(1)));
	return true;
}

INTRINSIC(gt_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) > GetIntVal(1)));
	return true;
}

INTRINSIC(le_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) <= GetIntVal(1)));
	return true;
}

INTRINSIC(ge_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) >= GetIntVal(1)));
	return true;
}

INTRINSIC(ne_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0) != GetIntVal(1)));
	return true;
}

INTRINSIC(logand_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) && GetFltVal(1)));
	return true;
}

INTRINSIC(logor_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) || GetFltVal(1)));
	return true;
}

INTRINSIC(eq_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) == GetFltVal(1)));
	return true;
}

INTRINSIC(lt_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) < GetFltVal(1)));
	return true;
}

INTRINSIC(gt_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) > GetFltVal(1)));
	return true;
}

INTRINSIC(le_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) <= GetFltVal(1)));
	return true;
}

INTRINSIC(ge_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) >= GetFltVal(1)));
	return true;
}

INTRINSIC(ne_flt)
{
	stmt->setVal(CreateFltVal(GetFltVal(0) != GetFltVal(1)));
	return true;
}

INTRINSIC(uadd_int)
{
	stmt->setVal(CreateIntVal(+GetIntVal(0)));
	return true;
}

INTRINSIC(usub_int)
{
	stmt->setVal(CreateIntVal(-GetIntVal(0)));
	return true;
}

INTRINSIC(lognot_int)
{
	stmt->setVal(CreateIntVal(!GetIntVal(0)));
	return true;
}

INTRINSIC(bnot_int)
{
	stmt->setVal(CreateIntVal(~GetIntVal(0)));
	return true;
}

INTRINSIC(incx_int)
{
	stmt->setVal(CreateIntVal(++GetIntVal(0)));
	return true;
}

INTRINSIC(decx_int)
{
	stmt->setVal(CreateIntVal(--GetIntVal(0)));
	return true;
}

INTRINSIC(uadd_flt)
{
	stmt->setVal(CreateFltVal(+GetFltVal(0)));
	return true;
}

INTRINSIC(usub_flt)
{
	stmt->setVal(CreateFltVal(-GetFltVal(0)));
	return true;
}

INTRINSIC(lognot_flt)
{
	stmt->setVal(CreateFltVal(!GetFltVal(0)));
	return true;
}

INTRINSIC(xinc_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0)++));
	return true;
}

INTRINSIC(xdec_int)
{
	stmt->setVal(CreateIntVal(GetIntVal(0)--));
	return true;
}
} // namespace sc