#include <cstring>
#include <iostream>

#include "Args.hpp"
#include "AST/Parser.hpp"
#include "AST/Passes/IRGen.hpp"
#include "Builder.hpp"
#include "FS.hpp"
#include "Logger.hpp"

using namespace sc;

// Uses task to determine build/run/install/etc.
int BuildProject(args::ArgParser &args, StringRef task);

int CompileFile(args::ArgParser &args, String &file);
// This function is provided as a callback to the VM since file parsing is supposed to be a
// black box for it, but is required by the import() function.
// Modifies path to absolute path
bool ParseFile(Allocator &allocator, args::ArgParser &args, String &path);

// CompileFile minus the file reading part.
// In this function, the filesystem is never touched, therefore path can be something like <repl>.
int CompileSource(args::ArgParser &args, StringRef path, StringRef code);
// ParseFile minus the file reading part.
// In this function, the filesystem is never touched, therefore path can be something like <repl>.
bool ParseSource(Allocator &allocator, args::ArgParser &args, StringRef path, StringRef code);

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
		args.printHelp(std::cout);
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
		file = "build";
		logger.fatal("Error: no source provided to read from");
		return 1;
	}

	if(file == "build" || file == "run" || file == "install") {
		return BuildProject(args, file);
	}
	return CompileFile(args, file);
}

int BuildProject(args::ArgParser &args, StringRef task)
{
	std::error_code ec;
	if(!fs::mkdir("build", ec)) return ec.value();

	if(!CompileSource(args, "<build>", buildcode)) {
		logger.fatal("Failed to compile build code");
		return 1;
	}

	// Compilation Driver (C, LLVM, ...)

	// Execute built binary
	return 0;
}

int CompileFile(args::ArgParser &args, String &path)
{
	// TODO: get allocator from a VM instance created here
	Allocator allocator("<Should be VM Allocator>");

	if(!ParseFile(allocator, args, path)) return 1;

	// Compilation Driver (C, LLVM, ...)
	return 0;
}

bool ParseFile(Allocator &allocator, args::ArgParser &args, String &path)
{
	if(!fs::exists(path)) {
		logger.fatal("File does not exist: ", path);
		return false;
	}
	path = fs::absPath(path.c_str());

	String data;
	int totalLinesInFile = 0;
	if(!fs::read(path.c_str(), data, &totalLinesInFile)) {
		logger.fatal("Failed to read file: ", path);
		return false;
	}

	return ParseSource(allocator, args, path, data);
}

int CompileSource(args::ArgParser &args, StringRef path, StringRef code)
{
	// TODO: get allocator from a VM instance created here
	Allocator allocator("<Should be VM Allocator>");

	if(!ParseSource(allocator, args, path, code)) return 1;

	// Compilation Driver (C, LLVM, ...)
	return 0;
}

bool ParseSource(Allocator &allocator, args::ArgParser &args, StringRef path, StringRef code)
{
	size_t moduleId = ModuleLoc::getOrAddModuleIdForPath(path);

	Vector<lex::Lexeme> tokens;
	if(!lex::tokenize(moduleId, code, tokens)) {
		logger.fatal("Failed to tokenize file: ", path);
		return false;
	}
	if(args.has("tokens")) {
		std::cout << "====================== Tokens for: " << path
			  << " ======================\n";
		lex::dumpTokens(std::cout, tokens);
	}

	// Separate allocator for AST since we don't want the AST nodes (Stmt) to persist outside
	// this function - because this function is supposed to generate IR for the VM to consume.
	Allocator astallocator(utils::toString("AST(", path, ")"));
	ast::StmtBlock *ptree = nullptr;
	if(!ast::parse(astallocator, tokens, ptree)) {
		logger.fatal("Failed to parse tokens for file: ", path);
		return false;
	}
	if(args.has("ast")) {
		std::cout << "====================== AST for: " << path
			  << " ======================\n";
		ast::dumpTree(std::cout, ptree);
	}

	ast::PassManager pm;
	Vector<Value *> ir;
	pm.add<ast::IRGenPass>(allocator, ir);

	if(!pm.visit((ast::Stmt *&)ptree)) {
		logger.fatal("Failed to perform passes on AST for file: ", path);
		return false;
	}

	// IR is ready now

	return true;
}