#include <cstring>
#include <iostream>

#include "Args.hpp"
#include "Builder.hpp"
#include "FS.hpp"
#include "Logger.hpp"
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
	args.add("trace").setShort("T").setHelp("show trace (even more verbose) compiler output");
	args.parse();

	if(args.has("help")) {
		args.printHelp(stdout);
		return 0;
	}

	if(args.has("version")) {
		std::cout << PROJECT_NAME << " compiler " << COMPILER_MAJOR << '.' << COMPILER_MINOR
			  << '.' << COMPILER_PATCH << '(' << REPO_URL << ' ' << COMMIT_ID << " ["
			  << TREE_STATUS << "])\nBuilt with " << BUILD_COMPILER << "\nOn "
			  << BUILD_DATE << '\n';
		return 0;
	}

	logger.addSink(&std::cerr, true, false);
	if(args.has("verbose")) logger.setLevel(LogLevels::INFO);
	else if(args.has("trace")) logger.setLevel(LogLevels::TRACE);

	String file = String(args.get(1));
	if(file.empty()) {
		logger.fatal("Error: no source provided to read from");
		return 1;
	}

	if(file == "build" || file == "run") {
		return BuildRunProj(args, file == "build");
	}
	return CompileFile(args, file);
}

int BuildRunProj(args::ArgParser &args, bool buildonly)
{
	std::error_code ec;
	if(!fs::mkdir("build", ec)) return 1;

	int res	    = 0;
	String file = "<build>";
	RAIIParser parser(args);
	if(!parser.init()) return 1;
	if(!parser.parse(file, true, buildcode)) return 1;

	if(args.has("tokens")) parser.dumpTokens();
	if(args.has("ast")) parser.dumpParseTree();

	if(args.has("nofile")) return 0;

	// Compilation Driver (C, LLVM, ...)

	// Execute built binary
	return res;
}

int CompileFile(args::ArgParser &args, String &file)
{
	if(!fs::exists(file)) {
		logger.fatal("Error: file ", file, " does not exist");
		return 1;
	}
	file = fs::absPath(file.c_str());

	RAIIParser parser(args);
	if(!parser.init()) return 1;
	if(!parser.parse(file, true)) return 1;

	if(args.has("tokens")) parser.dumpTokens();
	if(args.has("ast")) parser.dumpParseTree();

	if(args.has("nofile")) return 0;

	// Compilation Driver (C, LLVM, ...)
	return 0;
}