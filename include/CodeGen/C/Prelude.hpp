/*
	MIT License
	Copyright (c) 2021 Scribe Language Repositories
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef CODEGEN_C_PRELUDE_HPP
#define CODEGEN_C_PRELUDE_HPP

#include <string>
#include <vector>

namespace sc
{
// required for, say, setenv()
static const std::vector<std::string> default_preheadermacros = {"#define _DEFAULT_SOURCE"};
static const std::vector<std::string> default_includes = {"<stdio.h>", "<inttypes.h>", "<stdlib.h>",
							  "<stdbool.h>"};

static const std::vector<std::string> default_typedefs = {
"typedef bool i1;",	 "typedef char i8;",	"typedef int16_t i16;",	 "typedef int32_t i32;",
"typedef int64_t i64;",	 "typedef uint8_t u8;", "typedef uint16_t u16;", "typedef uint32_t u32;",
"typedef uint64_t u64;", "typedef float f32;",	"typedef double f64;"};
} // namespace sc

#endif // CODEGEN_C_PRELUDE_HPP