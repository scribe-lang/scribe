#pragma once

#include "Types.hpp"

namespace sc
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Core
///////////////////////////////////////////////////////////////////////////////////////////////////

INTRINSIC(compilerid);
INTRINSIC(compilerpath);
INTRINSIC(import);
INTRINSIC(setstringrefty);
INTRINSIC(ismainsrc);
INTRINSIC(isprimitive);
INTRINSIC(isptr);
INTRINSIC(isprimitiveorptr);
INTRINSIC(isint);
INTRINSIC(isintsigned);
INTRINSIC(isflt);
INTRINSIC(iscchar);
INTRINSIC(iscstring);
INTRINSIC(isequalty);
INTRINSIC(as);
INTRINSIC(typeof);
INTRINSIC(ptr);
INTRINSIC(szof);
INTRINSIC(valen);
INTRINSIC(array);
INTRINSIC(enumtagty);
INTRINSIC(assn_ptr);
INTRINSIC(getosid);
INTRINSIC(syspathmax);
INTRINSIC(compileerror);
INTRINSIC(setmaxcompilererr);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Primitive Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

// Int

INTRINSIC(assn_int);

INTRINSIC(add_int);
INTRINSIC(sub_int);
INTRINSIC(mul_int);
INTRINSIC(div_int);
INTRINSIC(mod_int);
INTRINSIC(band_int);
INTRINSIC(bor_int);
INTRINSIC(bxor_int);
INTRINSIC(lshift_int);
INTRINSIC(rshift_int);

INTRINSIC(addassn_int);
INTRINSIC(subassn_int);
INTRINSIC(mulassn_int);
INTRINSIC(divassn_int);
INTRINSIC(modassn_int);
INTRINSIC(bandassn_int);
INTRINSIC(borassn_int);
INTRINSIC(bxorassn_int);
INTRINSIC(lshiftassn_int);
INTRINSIC(rshiftassn_int);

INTRINSIC(logand_int);
INTRINSIC(logor_int);
INTRINSIC(eq_int);
INTRINSIC(lt_int);
INTRINSIC(gt_int);
INTRINSIC(le_int);
INTRINSIC(ge_int);
INTRINSIC(ne_int);

INTRINSIC(uadd_int);
INTRINSIC(usub_int);
INTRINSIC(lognot_int);
INTRINSIC(bnot_int);

INTRINSIC(xinc_int);
INTRINSIC(incx_int);
INTRINSIC(xdec_int);
INTRINSIC(decx_int);

// Float

INTRINSIC(assn_flt);

INTRINSIC(add_flt);
INTRINSIC(sub_flt);
INTRINSIC(mul_flt);
INTRINSIC(div_flt);

INTRINSIC(addassn_flt);
INTRINSIC(subassn_flt);
INTRINSIC(mulassn_flt);
INTRINSIC(divassn_flt);

INTRINSIC(logand_flt);
INTRINSIC(logor_flt);
INTRINSIC(eq_flt);
INTRINSIC(lt_flt);
INTRINSIC(gt_flt);
INTRINSIC(le_flt);
INTRINSIC(ge_flt);
INTRINSIC(ne_flt);

INTRINSIC(uadd_flt);
INTRINSIC(usub_flt);
INTRINSIC(lognot_flt);
INTRINSIC(bnot_flt);

} // namespace sc
