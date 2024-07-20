#include "CompilerContext.hpp"

#include "AST/Passes/Base.hpp"

namespace sc
{
Context::Context(RAIIParser *parser) : parser(parser) {}
Context::~Context()
{
#ifdef MEM_COUNT // from Core.hpp
	size_t m1 = 0;
#endif
	for(auto &s : stmtmem) {
#ifdef MEM_COUNT
		++m1;
#endif
		delete s;
	}
#ifdef MEM_COUNT
	printf("Total compiler context deallocations:\nStmts: %zu\n", m1);
#endif
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
