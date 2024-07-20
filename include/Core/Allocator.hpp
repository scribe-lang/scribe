#pragma once

#include "Core.hpp"

namespace sc
{

// Base class for anything that uses the allocator
class IAllocated
{
public:
	IAllocated();
	virtual ~IAllocated();
};

// Cannot be a static object - as it uses the static variable `logger` in destructor
class Allocator
{
	UniList<IAllocated *> allocs;
	String name;

public:
	Allocator(StringRef name);
	~Allocator();

	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<IAllocated, T>::value, T *>::type
	alloc(Args... args)
	{
		T *res = new T(args...);
		allocs.push_front(res);
		return res;
	}
};

} // namespace sc