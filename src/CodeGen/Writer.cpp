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
	dest += String(indent, '\t');
}

void Writer::append(Writer &other)
{
	dest += other.getData();
}

void Writer::write(StringRef data)
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
	dest += "'" + getRawString(String(1, data)) + "'";
}
void Writer::writeConstString(StringRef data)
{
	dest += "\"" + getRawString(data) + "\"";
}

void Writer::writeBefore(StringRef data)
{
	dest.insert(dest.begin(), data.begin(), data.end());
}
void Writer::insertAfter(const size_t &pos, StringRef data)
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

String &Writer::getData()
{
	return dest;
}

const size_t &Writer::getIndent()
{
	return indent;
}
} // namespace sc