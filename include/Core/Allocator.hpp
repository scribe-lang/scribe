#pragma once

// Named allocator template

#include "Logger.hpp"

namespace sc
{

// Cannot be a static object - as it uses the static variable `logger` in destructor

template<typename Ty> class ListAllocator
{
	UniList<Ty *> allocs;
	String name;

public:
	ListAllocator(StringRef name) : name(name) {}
	~ListAllocator()
	{
		size_t count = 0;
		for(auto &m : allocs) {
			++count;
			delete m;
		}
		logger.trace("ListAllocator '", name, "' contained ", count, " allocations");
	}

	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Ty, T>::value, T *>::type alloc(Args... args)
	{
		T *res = new T(args...);
		allocs.push_front(res);
		return res;
	}
};

} // namespace sc