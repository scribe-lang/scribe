#pragma once

#include "Core.hpp"

namespace sc
{

// required for, say, setenv()
static const Vector<StringRef> default_preheadermacros = {"#define _DEFAULT_SOURCE"};
static const Vector<StringRef> default_includes	       = {"<stdio.h>", "<inttypes.h>", "<stdlib.h>",
							  "<stdbool.h>"};

static const Vector<StringRef> default_typedefs = {
"typedef bool i1;",	 "typedef char i8;",	"typedef int16_t i16;",	 "typedef int32_t i32;",
"typedef int64_t i64;",	 "typedef uint8_t u8;", "typedef uint16_t u16;", "typedef uint32_t u32;",
"typedef uint64_t u64;", "typedef float f32;",	"typedef double f64;"};

static const StringRef default_macro_magic = "#if defined(_SYS_STAT_H) && defined(__USE_XOPEN2K8)\n\
#define st_atimensec st_atim.tv_nsec\n\
#define st_mtimensec st_mtim.tv_nsec\n\
#define st_ctimensec st_ctim.tv_nsec\n\
#endif\n\
\n\
#define _SC_INLINE_ __attribute__((always_inline)) inline\
";

} // namespace sc
