/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifdef __APPLE__
	// for basename() on macOS
	#include <libgen.h>
#endif // __APPLE__
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "Args.hpp"
#include "Builder.hpp"
#include "CodeGen/C.hpp"
#include "Config.hpp"
#include "Error.hpp"
#include "FS.hpp"
#include "Lex.hpp"
#include "Parser.hpp"

using namespace sc;

int BuildRunProj(args::ArgParser &args, bool buildonly);
int CompileFile(args::ArgParser &args, String &file);

int main(int argc, char **argv)
{
	args::ArgParser args(argc, (const char **)argv);
	args.add("version").setShort("v").setHelp("prints program version");
	args.add("tokens").setShort("t").setHelp("shows lexical tokens");
	args.add("parse").setShort("p").setHelp("shows AST");
	args.add("semantic").setShort("s").setHelp("shows Semantic Tree");
	args.add("ir").setShort("i").setHelp("shows codegen IR");
	args.add("nofile").setShort("n").setHelp("disables output to a file");
	args.add("opt").setShort("O").setValReqd(true).setHelp("set optimization level");
	args.add("std").setShort("std").setValReqd(true).setHelp("set C standard");
	args.add("llir").setShort("llir").setHelp("emit LLVM IR (C backend)");
	args.add("verbose").setShort("V").setHelp("show verbose compiler output");
	args.parse();

	if(args.has("help")) {
		args.printHelp(stdout);
		return 0;
	}

	if(args.has("version")) {
		fprintf(stdout, "%s compiler %d.%d.%d (%s %s [%s])\nBuilt with %s\nOn %s\n",
			PROJECT_NAME, COMPILER_MAJOR, COMPILER_MINOR, COMPILER_PATCH, REPO_URL,
			COMMIT_ID, TREE_STATUS, BUILD_CXX_COMPILER, BUILD_DATE);
		return 0;
	}
	String file = String(args.get(1));
	if(file.empty()) {
		std::cerr << "Error: no source provided to read from\n";
		return 1;
	}

	if(file == "build" || file == "run") {
		return BuildRunProj(args, file == "build");
	}
	return CompileFile(args, file);
}

int BuildRunProj(args::ArgParser &args, bool buildonly)
{
	int res = std::system("mkdir -p build");
	res	= WEXITSTATUS(res);
	if(res) return res;

	String file = "<build>";
	RAIIParser parser(args);
	if(!parser.parse(file, true, buildcode)) return 1;
	parser.dumpTokens(false);
	parser.dumpParseTree(false);
	if(args.has("nofile")) return 0;

	CDriver cdriver(parser);
	StringRef outfile = "./build/builder";
	if(!cdriver.compile(outfile)) return 1;
	String cmd = "./build/builder .";
	auto argv  = args.getArgv();
	// append everything to cmd after build/run
	for(int i = 1; i < argv.size(); ++i) {
		cmd += " ";
		cmd += argv[i];
	}
	res = std::system(cmd.c_str());
	res = WEXITSTATUS(res);
	return res;
}

int CompileFile(args::ArgParser &args, String &file)
{
	if(!fs::exists(file)) {
		std::cerr << "Error: file " << file << " does not exist\n";
		return 1;
	}
	file = fs::absPath(file);

	RAIIParser parser(args);
	if(!parser.parse(file, true)) return 1;
	parser.dumpTokens(false);
	parser.dumpParseTree(false);
	if(args.has("nofile")) return 0;

	// TODO: make proper log system
	if(args.has("verbose")) std::cout << "total read lines: " << fs::getTotalLines() << "\n";

	CDriver cdriver(parser);
	String outfile = String(args.get(2));
	if(outfile.empty()) {
		char f[2048] = {0};
		strcpy(f, file.c_str());
		outfile = basename(f);
	}
	auto ext = outfile.find_last_of('.');
	if(ext != String::npos) outfile = outfile.substr(0, ext);
	if(!cdriver.compile(outfile)) return 1;
	return 0;
}