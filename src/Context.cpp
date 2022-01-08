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

namespace sc
{
Context::Context(RAIIParser *parser) : parser(parser) {}
Context::~Context()
{
	// size_t s1 = 0, l1 = 0, s2 = 0, t1 = 0, v1 = 0;
	// for(auto &s : stringmem) ++s1;
	for(auto &l : modlocmem) {
		// ++l1;
	}
	for(auto &s : stmtmem) {
		// ++s2;
		delete s;
	}
	for(auto &t : typemem) {
		// ++t1;
		delete t;
	}
	for(auto &v : valmem) {
		// ++v1;
		delete v;
	}
	// printf("Total deallocation:\nStrings: %zu\nModLocs:"
	//        " %zu\nStmts: %zu\nTypes: %zu\nVals: %zu\n",
	//        s1, l1, s2, t1, v1);
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
StringRef Context::strFrom(int64_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
StringRef Context::strFrom(size_t i)
{
	stringmem.push_front(std::to_string(i));
	return stringmem.front();
}
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
