/*
	MIT License
	Copyright (c) 2022 Scribe Language Repositories
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef CODEGEN_WRITER_HPP
#define CODEGEN_WRITER_HPP

#include "Core.hpp"

namespace sc
{
class Writer
{
	String dest;
	size_t indent;

public:
	Writer();
	Writer(Writer &other);
	void addIndent(const size_t &count = 1);
	void remIndent(const size_t &count = 1);

	// adds '\n' and appends indentation
	void newLine();

	void append(Writer &other);

	void write(const char *data, ...);
	void write(StringRef data);
	void write(const int64_t &data);
	void write(const double &data);
	void writeConstChar(const int64_t data);
	void writeConstString(StringRef data);
	void writeBefore(const char *data, ...);
	void writeBefore(StringRef data);
	void insertAfter(const size_t &pos, StringRef data);

	inline void write(InitList<StringRef> data)
	{
		for(auto &d : data) write(d);
	}

	inline void writeBefore(InitList<StringRef> data)
	{
		for(auto &d : data) writeBefore(d);
	}

	void reset(Writer &other);
	void clear();
	bool empty();

	String &getData();
	const size_t &getIndent();
};
} // namespace sc

#endif // CODEGEN_WRITER_HPP