#include "Args.hpp"
#include "Env.hpp"
#include "FS.hpp"
#include "Logger.hpp"

using namespace sc;

int main(int argc, char **argv)
{
	ArgParser args(argc, (const char **)argv);
	args.add("version").setShort("v").setHelp("prints program version");
	args.add("tokens").setShort("t").setHelp("shows lexical tokens");
	args.add("parse").setShort("p").setHelp("shows AST");
	args.add("optparse").setShort("P").setHelp("shows optimized AST (AST after passes)");
	args.add("ir").setShort("i").setHelp("shows codegen IR");
	args.add("dry").setShort("d").setHelp("dry run - generate IR but don't run the VM");
	args.add("logerr").setShort("e").setHelp("show logs on stderr");
	args.add("verbose").setShort("V").setHelp("show verbose compiler output");
	args.add("trace").setShort("T").setHelp("show trace (even more verbose) compiler output");
	if(!args.parse()) return 1;

	if(args.has("help")) {
		args.printHelp(std::cout);
		return 0;
	}

	if(args.has("version")) {
		std::cout << PROJECT_NAME << " " << PROJECT_MAJOR << "." << PROJECT_MINOR << "."
			  << PROJECT_PATCH << " (" << REPO_URL << " " << COMMIT_ID << " "
			  << TREE_STATUS << ")\nBuilt with " << BUILD_COMPILER << "\nOn "
			  << BUILD_DATE << "\n";
		return 0;
	}

	if(args.has("logerr")) logger.addSink(&std::cerr, true, false);
	if(args.has("verbose")) logger.setLevel(LogLevels::INFO);
	else if(args.has("trace")) logger.setLevel(LogLevels::TRACE);

	if(args.getSource().empty()) {
		// args.setSource("<repl>");
		// return ExecInteractive(args);
		std::cout << "FATAL: Unimplemented interactive mode\n";
		return 1;
	}

	const char *file = args.getSource().c_str();

	if(!fs::exists(file)) {
		String binfile(fs::parentDir(env::getProcPath()));
#if defined(CORE_OS_WINDOWS)
		binfile += "\\";
#else
		binfile += "/";
#endif
		binfile += file;
		binfile += ".fer";
		if(!fs::exists(binfile)) {
			err.fail({}, "File ", file, " does not exist");
			return 1;
		}
		args.setSource(binfile);
		file = args.getSource().c_str();
	}

	return 0;
	// Interpreter ip(args, ParseSource);
	// return ip.runFile({}, fs::absPath(file).c_str());
}