#pragma once

#include "Args.hpp"
#include "AST/Passes/Base.hpp"

namespace sc
{

class Module
{
	Context &ctx;

	String id;
	String path;
	String code;
	Vector<lex::Lexeme> tokens;
	AST::Stmt *ptree;
	// Vector<Instruction *> instrs;
	bool is_main_module;

public:
	Module(Context &ctx, String &&id, const String &path, StringRef code, bool is_main_module);
	Module(Context &ctx, String &&id, const String &path, String &&code, bool is_main_module);
	~Module();

	bool tokenize();
	bool parseTokens();

	// Allocates instructions
	// This is the only function where instructions can be allocated
	// template<typename T, typename... Args>
	// std::enable_if<std::is_base_of<Instruction, T>::value, T *>::type
	// appendInstruction(Args &&...args)
	// {
	// 	instrs.push_back(new T(std::forward<Args>(args)...));
	// 	return instrs.back();
	// }

	inline bool executePasses(AST::PassManager &pm) { return pm.visit(ptree); }

	inline StringRef getID() const { return id; }
	inline StringRef getPath() const { return path; }
	inline StringRef getCode() const { return code; }
	inline Span<const lex::Lexeme> getTokens() const { return tokens; }
	inline bool isMainModule() const { return is_main_module; }
	inline AST::Stmt *&getParseTree() { return ptree; }
	// inline Span<Instruction *> getInstructions() { return instrs; }
	// inline Instruction *getInstruction(size_t idx)
	// {
	// 	return idx < instrs.size() ? instrs[idx] : nullptr;
	// }

	void dumpTokens() const;
	void dumpParseTree() const;
	void dumpIR() const;
};

class RAIIParser
{
	args::ArgParser &args;

	Context ctx;

	// default pms that run:
	// 1. on each module
	// 2. once all modules are combined
	AST::PassManager defaultpmpermodule, defaultpmcombined;

	// as new sources are imported, they'll be pushed back
	Vector<StringRef> modulestack;

	// the iteration of this list will give the order of imports
	// this list does NOT contain the main module
	Vector<StringRef> moduleorder;

	Map<StringRef, Module *> modules;

	Module *mainmodule;

	Module *addModule(const String &path, bool main_module, StringRef code);

public:
	RAIIParser(args::ArgParser &args);
	~RAIIParser();

	bool init();

	// if code is not empty, file won't be read/checked
	bool parse(const String &path, bool main_module = false, StringRef code = "");
	void combineAllModules();

	inline bool hasModule(StringRef path) { return modules.find(path) != modules.end(); }
	inline const Vector<StringRef> &getModuleStack() { return modulestack; }
	inline args::ArgParser &getCliArgs() { return args; }
	inline Context &getContext() { return ctx; }
	inline Module *getMainModule() { return mainmodule; }
	Module *getModule(StringRef path);

	// force ignores arg parser
	void dumpTokens();
	void dumpParseTree();

	// search for a source (import) in the import/include paths
	// updates the modname parameter if the file is found
	bool isValidSource(String &modname);
};
} // namespace sc
