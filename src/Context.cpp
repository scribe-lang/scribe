/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Context.hpp"

#include "Passes/Base.hpp"
#include "Types.hpp"
#include "Values.hpp"

// #define MEM_COUNT

namespace sc
{
Context::Context(RAIIParser *parser) : parser(parser) {}
Context::~Context()
{
#ifdef MEM_COUNT
	size_t s1 = 0, l1 = 0, s2 = 0, t1 = 0, v1 = 0;
	for(auto &s : stringmem) ++s1;
#endif
	for(auto &l : modlocmem) {
#ifdef MEM_COUNT
		++l1;
#endif
	}
	for(auto &s : stmtmem) {
#ifdef MEM_COUNT
		++s2;
#endif
		delete s;
	}
	for(auto &t : typemem) {
#ifdef MEM_COUNT
		++t1;
#endif
		delete t;
	}
	for(auto &v : valmem) {
#ifdef MEM_COUNT
		++v1;
#endif
		delete v;
	}
#ifdef MEM_COUNT
	printf("Total deallocation:\nStrings: %zu\nModLocs:"
	       " %zu\nStmts: %zu\nTypes: %zu\nVals: %zu\n",
	       s1, l1, s2, t1, v1);
#endif
}

StringRef Context::strFrom(InitList<StringRef> strs)
{
	stringmem.push_front({});
	String &res = stringmem.front();
	for(auto &s : strs) {
		res += s;
	}
	return res;
}
StringRef Context::strFrom(const String &s)
{
	stringmem.push_front(s);
	return stringmem.front();
}
StringRef Context::moveStr(String &&str)
{
	stringmem.push_front(std::move(str));
	return stringmem.front();
}
StringRef Context::strFrom(int32_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
StringRef Context::strFrom(int64_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
StringRef Context::strFrom(uint32_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
StringRef Context::strFrom(size_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
#ifdef __APPLE__
StringRef Context::strFrom(uint64_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
#endif // __APPLE__
ModuleLoc *Context::allocModuleLoc(Module *mod, size_t line, size_t col)
{
	modlocmem.emplace_front(mod, line, col);
	return &modlocmem.front();
}

void Context::addPass(const size_t &id, Pass *pass)
{
	passes[id] = pass;
}
void Context::remPass(const size_t &id)
{
	auto loc = passes.find(id);
	if(loc != passes.end()) passes.erase(loc);
}
Pass *Context::getPass(const size_t &id)
{
	auto loc = passes.find(id);
	if(loc != passes.end()) return loc->second;
	return nullptr;
}
} // namespace sc
