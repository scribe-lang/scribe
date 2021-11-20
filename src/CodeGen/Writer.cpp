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

#include "CodeGen/Writer.hpp"

#include <cstring>

#include "Utils.hpp"

namespace sc
{
Writer::Writer() : indent(0) {}
Writer::Writer(Writer &other) : indent(other.indent) {}

void Writer::addIndent(const size_t &count)
{
	indent += count;
}
void Writer::remIndent(const size_t &count)
{
	indent -= count;
}

// adds '\n' and appends indentation
void Writer::newLine()
{
	dest += "\n";
	dest += std::string(indent, '\t');
}

void Writer::append(Writer &other)
{
	dest += other.getData();
}

void Writer::write(const char *data, ...)
{
	static char dat[4096];
	std::memset(dat, 0, 4096);
	va_list args;
	va_start(args, data);
	vsprintf(dat, data, args);
	va_end(args);
	dest += dat;
}

void Writer::write(const std::string &data)
{
	dest += data;
}

void Writer::write(const int64_t &data)
{
	dest += std::to_string(data);
}
void Writer::write(const double &data)
{
	dest += std::to_string(data);
}
void Writer::writeConstChar(const int64_t data)
{
	dest += "'" + getRawString(std::string(1, data)) + "'";
}
void Writer::writeConstString(const std::string &data)
{
	dest += "\"" + getRawString(data) + "\"";
}

void Writer::writeBefore(const char *data, ...)
{
	static char dat[4096];
	std::memset(dat, 0, 4096);
	va_list args;
	va_start(args, data);
	vsprintf(dat, data, args);
	va_end(args);
	dest = dat + dest;
}
void Writer::writeBefore(const std::string &data)
{
	dest = data + dest;
}
void Writer::insertAfter(const size_t &pos, const std::string &data)
{
	dest.insert(pos, data);
}

void Writer::reset(Writer &other)
{
	dest.clear();
	indent = other.indent;
}
void Writer::clear()
{
	dest.clear();
}
bool Writer::empty()
{
	return dest.empty();
}

const std::string &Writer::getData()
{
	return dest;
}

const size_t &Writer::getIndent()
{
	return indent;
}
} // namespace sc