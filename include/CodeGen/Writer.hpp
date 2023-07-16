#pragma once

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
	void addIndent(size_t count = 1);
	void remIndent(size_t count = 1);

	// adds '\n' and appends indentation
	void newLine();

	void append(Writer &other);

	void write(StringRef data);
	void write(uint32_t data);
	void write(int64_t data);
	void write(const double &data);
	void write(size_t count, char data);
	void writeConstChar(int64_t data);
	void writeConstString(StringRef data);
	void writeBefore(StringRef data);
	void writeBefore(size_t count, char data);
	void insertAfter(size_t pos, StringRef data);

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
	size_t getIndent();
};
} // namespace sc
