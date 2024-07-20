#include "VMContext.hpp"

#include "Values.hpp"

// #define MEM_COUNT

namespace sc
{

Context::Context() {}
Context::~Context()
{
#ifdef MEM_COUNT // from Core.hpp
	size_t m1 = 0;
#endif
	for(auto &v : valuemem) {
#ifdef MEM_COUNT
		++m1;
#endif
		delete v;
	}
#ifdef MEM_COUNT
	printf("Total VM context deallocations:\nValues: %zu\n", m1);
#endif
}

} // namespace sc
