#pragma once

#include "Core.hpp"

namespace sc
{
namespace AST
{
class Stmt;
class Pass;
} // namespace AST
class RAIIParser;
class Module;
class ModuleLoc;
class Context
{
	UniList<ModuleLoc> modlocmem;
	Vector<AST::Stmt *> stmtmem;
	Map<size_t, AST::Pass *> passes;
	RAIIParser *parser;

public:
	Context(RAIIParser *parser);
	~Context();

	ModuleLoc *allocModuleLoc(size_t moduleId, size_t offset);

	template<typename T, typename... Args> T *allocStmt(Args... args)
	{
		T *res = new T(args...);
		stmtmem.push_back(res);
		return res;
	}

	void addPass(size_t id, AST::Pass *pass);
	void remPass(size_t id);
	AST::Pass *getPass(size_t id);
	template<typename T>
	typename std::enable_if<std::is_base_of<AST::Pass, T>::value, T *>::type getPass()
	{
		return static_cast<T *>(getPass(T::template genPassID<T>()));
	}

	inline RAIIParser *getParser() { return parser; }
};
} // namespace sc
