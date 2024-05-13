#include "Context.hpp"

#include "AST/Passes/Base.hpp"

// #define MEM_COUNT

namespace sc
{
Context::Context(RAIIParser *parser) : parser(parser) {}
Context::~Context()
{
#ifdef MEM_COUNT
	size_t s1 = 0, l1 = 0, s2 = 0;
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
#ifdef MEM_COUNT
	printf("Total deallocation:\nStrings: %zu\nModLocs:"
	       " %zu\nStmts: %zu\n",
	       s1, l1, s2);
#endif
}

ModuleLoc *Context::allocModuleLoc(Module *mod, size_t line, size_t col)
{
	modlocmem.emplace_front(mod, line, col);
	return &modlocmem.front();
}

void Context::addPass(size_t id, AST::Pass *pass) { passes[id] = pass; }
void Context::remPass(size_t id)
{
	auto loc = passes.find(id);
	if(loc != passes.end()) passes.erase(loc);
}
AST::Pass *Context::getPass(size_t id)
{
	auto loc = passes.find(id);
	if(loc != passes.end()) return loc->second;
	return nullptr;
}
} // namespace sc
