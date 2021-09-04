/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <vector>

namespace sc
{
class Type;
class Value;
class Context
{
	std::vector<Type *> typemem;
	std::vector<Value *> valmem;

public:
	Context();
	~Context();

	template<typename T, typename... Args> T *allocType(Args... args)
	{
		T *res = new T(args...);
		typemem.push_back(res);
		return res;
	}
	template<typename T, typename... Args> T *allocVal(Args... args)
	{
		T *res = new T(args...);
		valmem.push_back(res);
		return res;
	}
};
} // namespace sc

#endif // CONTEXT_HPP