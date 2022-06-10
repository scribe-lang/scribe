/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the \\"Software\\"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef BUILDER_HPP
#define BUILDER_HPP

#include "Core.hpp"

namespace sc
{
static const StringRef buildcode = "\
let io = @import(\"std/io\");\n\
let err = @import(\"std/err\");\n\
let argparse = @import(\"std/argparse\");\n\
let builder = @import(\"std/builder\");\n\
\n\
let build = @import(\"./build\");\n\
\n\
let main = fn(argc: i32, argv: **i8): i32 {\n\
	err.init();\n\
	defer err.deinit();\n\
\n\
	let args = argparse.new(argc, argv);\n\
	defer args.deinit();\n\
\n\
	builder.initArgParser(args);\n\
\n\
	if !args.parse() {\n\
		io.println(\"Failed to parse command line arguments\");\n\
		if err.present() {\n\
			io.println(\"Error: \", err.pop());\n\
		}\n\
		return 1;\n\
	}\n\
\n\
	let b = builder.new(args);\n\
	defer b.deinit();\n\
	let res = b.setup();\n\
	if err.present() {\n\
		io.println(\"Build setup failed: \", err.pop());\n\
		return !res;\n\
	}\n\
	res = b.perform();\n\
	if err.present() {\n\
		io.println(\"Build execution failed: \", err.pop());\n\
	}\n\
\n\
	return !res;\n\
};\n\
";
} // namespace sc

#endif // BUILDER_HPP