#pragma once

#include "Core.hpp"

namespace sc
{

class Value;

class Context
{
	UniList<Value *> valuemem;

public:
	Context();
	~Context();

	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Value, T>::value, T *>::type
	allocValue(Args... args)
	{
		T *res = new T(args...);
		valuemem.push_front(res);
		return res;
	}
};

} // namespace sc