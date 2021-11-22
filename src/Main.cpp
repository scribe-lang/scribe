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

#include <cstdio>
#include <cstring>
#include <stdexcept>

#include "Args.hpp"
#include "CodeGen/C.hpp"
#include "Config.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Lex.hpp"
#include "Parser.hpp"

int main(int argc, char **argv)
{
	using namespace sc;
	args::ArgParser args(argc, (const char **)argv);
	args.add("version").set_short("v").set_help("prints program version");
	args.add("tokens").set_short("t").set_help("shows lexical tokens");
	args.add("parse").set_short("p").set_help("shows AST");
	args.add("semantic").set_short("s").set_help("shows Semantic Tree");
	args.add("ir").set_short("i").set_help("shows codegen IR");
	args.add("nofile").set_short("n").set_help("disables output to a file");
	args.add("opt").set_short("O").set_val_reqd(true).set_help("set optimization level");
	args.add("std").set_short("std").set_val_reqd(true).set_help("set C standard");
	args.add("llir").set_short("llir").set_help("emit LLVM IR (C backend)");

	args.parse();

	if(args.has("help")) {
		args.print_help(stdout);
		return 0;
	}

	if(args.has("version")) {
		fprintf(stdout, "%s compiler %d.%d.%d (language %d.%d.%d)\nBuilt with %s\nOn %s\n",
			PROJECT_NAME, SCRIBE_MAJOR, SCRIBE_MINOR, COMPILER_MAJOR, COMPILER_MINOR,
			COMPILER_PATCH, SCRIBE_PATCH, BUILD_CXX_COMPILER, BUILD_DATE);
		return 0;
	}
	std::string file = args.get(1);
	if(file.empty()) {
		fprintf(stderr, "Error: no source provided to read from\n");
		return 1;
	}

	if(!fs::exists(file)) {
		fprintf(stderr, "Error: file %s does not exist\n", file.c_str());
		return 1;
	}
	file = fs::absPath(file);

	RAIIParser parser(args);
	if(!parser.parse(file, true)) return 1;
	parser.dumpTokens(false);
	parser.dumpParseTree(false);
	if(args.has("nofile")) return 0;

	CDriver cdriver(parser);
	std::string outfile = args.get(2);
	if(outfile.empty()) {
		char f[2048] = {0};
		strcpy(f, file.c_str());
		outfile = basename(f);
	}
	auto ext = outfile.find_last_of('.');
	if(ext != std::string::npos) outfile = outfile.substr(0, ext);
	if(!cdriver.compile(outfile)) return 1;
	return 0;
}