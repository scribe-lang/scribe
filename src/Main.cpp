#include <cstring>
#include <iostream>
#include <stdexcept>

#include "Args.hpp"
#include "Builder.hpp"
#include "Config.hpp"
#include "FS.hpp"
#include "Parser.hpp"

using namespace sc;

int BuildRunProj(args::ArgParser &args, bool buildonly);
int CompileFile(args::ArgParser &args, String &file);

int main(int argc, char **argv)
{
	args::ArgParser args(argc, (const char **)argv);
	args.add("version").setShort("v").setHelp("prints program version");
	args.add("tokens").setShort("t").setHelp("shows lexical tokens");
	args.add("ast").setShort("a").setHelp("shows abstract syntax tree");
	args.add("sst").setShort("s").setHelp("shows semantic syntax tree");
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
	if(!fs::mkdir("build")) return 1;

	int res	    = 0;
	String file = "<build>";
	RAIIParser parser(args);
	if(!parser.init()) return 1;
	if(!parser.parse(file, true, buildcode)) return 1;
	parser.dumpTokens(false);
	parser.dumpParseTree(false);
	if(args.has("nofile")) return 0;
	// TODO: execute builder
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
	if(!parser.init()) return 1;
	if(!parser.parse(file, true)) return 1;
	parser.dumpTokens(false);
	parser.dumpParseTree(false);
	if(args.has("nofile")) return 0;

	if(args.has("verbose")) {
		// TODO: make proper log system
		// logger.info("total read lines in source '%s': %d", file.c_str(),
		// 	    fs::getLastTotalLines());
		std::cout << "total read lines: " << fs::getLastTotalLines() << "\n";
	}

	// Compilation Driver (C, LLVM, ...)
	return 0;
}