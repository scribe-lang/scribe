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

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

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

inline bool startsWith(const std::string &src, const std::string &term)
{
	return src.rfind(term, 0) == 0;
}
template<typename T> inline bool isOneOf(const std::vector<T> &vec, const T &elem)
{
	for(auto &v : vec) {
		if(v == elem) return true;
	}
	return false;
}

// Also trims the spaces for each split
std::vector<std::string> stringDelim(const std::string &str, const std::string &delim);

// Convert special characters in string (\n, \t, ...) to raw (\\n, \\t, ...)
std::string getRawString(const std::string &data);
} // namespace sc

#endif // UTILS_HPP