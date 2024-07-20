#include "Allocator.hpp"

#include "Logger.hpp"

namespace sc
{

IAllocated::IAllocated() {}
IAllocated::~IAllocated() {}

Allocator::Allocator(StringRef name) : name(name) {}
Allocator::~Allocator()
{
	size_t count = 0;
	for(auto &m : allocs) {
		++count;
		delete m;
	}
	logger.trace(name, " allocator had ", count, " allocations");
}

} // namespace sc