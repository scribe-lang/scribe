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
	for(auto &l : modlocmem) delete l;
	for(auto &s : stmtmem) delete s;
	for(auto &t : typemem) delete t;
	for(auto &v : valmem) delete v;
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
