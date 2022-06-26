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

#ifndef UTILS_HPP
#define UTILS_HPP

#include "Core.hpp"

namespace sc
{
// RAII class to manage a pointer
template<typename T> class Pointer
{
	T *data;

public:
	Pointer(T *dat) : data(dat) {}
	~Pointer()
	{
		if(data) delete data;
	}
	void set(T *dat)
	{
		data = dat;
	}
	void unset()
	{
		data = nullptr;
	}
};

inline bool startsWith(StringRef src, StringRef term)
{
	return src.rfind(term, 0) == 0;
}

// Also trims the spaces for each split
Vector<StringRef> stringDelim(StringRef str, StringRef delim);

// Convert special characters in string (\n, \t, ...) to raw (\\n, \\t, ...)
void appendRawString(String &res, StringRef from);
} // namespace sc

#endif // UTILS_HPP