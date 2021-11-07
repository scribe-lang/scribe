/*
	MIT License
	Copyright (c) 2021 Scribe Language Repositories
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef CODEGEN_WRITER_HPP
#define CODEGEN_WRITER_HPP

#include <cstdarg>
#include <string>

namespace sc
{
class Writer
{
	std::string dest;
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
	void write(const std::string &data);
	void write(const int64_t &data);
	void write(const double &data);
	void writeConstChar(const int64_t data);
	void writeConstString(const std::string &data);
	void writeBefore(const char *data, ...);
	void writeBefore(const std::string &data);
	void insertAfter(const size_t &pos, const std::string &data);

	void reset(Writer &other);
	void clear();
	bool empty();

	const std::string &getData();
	const size_t &getIndent();
};
} // namespace sc

#endif // CODEGEN_WRITER_HPP