#pragma once

#include "Core.hpp"

namespace sc
{
class Stmt;
class Type;
class Value;
class Pass;
class RAIIParser;
class Module;
class ModuleLoc;
class Context
{
	List<String> stringmem;
	List<ModuleLoc> modlocmem;
	Vector<Stmt *> stmtmem;
	Vector<Type *> typemem;
	Vector<Value *> valmem;
	Map<size_t, Pass *> passes;
	RAIIParser *parser;

public:
	Context(RAIIParser *parser);
	~Context();

	StringRef strFrom(InitList<StringRef> strs);
	StringRef strFrom(const String &s);
	StringRef moveStr(String &&str);
	StringRef strFrom(int32_t i);
	StringRef strFrom(int64_t i);
	StringRef strFrom(uint32_t i);
	StringRef strFrom(size_t i);
#ifdef __APPLE__
	StringRef strFrom(uint64_t i);
#endif // __APPLE__
	ModuleLoc *allocModuleLoc(Module *mod, size_t line, size_t col);

	template<typename T, typename... Args> T *allocStmt(Args... args)
	{
		T *res = new T(args...);
		stmtmem.push_back(res);
		return res;
	}
	template<typename T, typename... Args> T *allocType(Args... args)
	{
		T *res = new T(args...);
		typemem.push_back(res);
		return res;
	}
	template<typename T, typename... Args> T *allocVal(Args... args)
	{
		T *res = new T(*this, args...);
		valmem.push_back(res);
		return res;
	}

	void addPass(size_t id, Pass *pass);
	void remPass(size_t id);
	Pass *getPass(size_t id);
	template<typename T>
	typename std::enable_if<std::is_base_of<Pass, T>::value, T *>::type getPass()
	{
		return static_cast<T *>(getPass(T::template genPassID<T>()));
	}

	inline RAIIParser *getParser() { return parser; }
};
} // namespace sc
