#include "CodeGen/Writer.hpp"

#include <cstring>

#include "Utils.hpp"

namespace sc
{
Writer::Writer() : indent(0) {}
Writer::Writer(Writer &other) : indent(other.indent) {}

void Writer::addIndent(size_t count) { indent += count; }
void Writer::remIndent(size_t count) { indent -= count; }

// adds '\n' and appends indentation
void Writer::newLine()
{
	dest += "\n";
	dest += String(indent, '\t');
}

void Writer::append(Writer &other) { dest += other.getData(); }

void Writer::write(StringRef data) { dest += data; }
void Writer::write(uint32_t data) { dest += std::to_string(data); }
void Writer::write(int64_t data) { dest += std::to_string(data); }
void Writer::write(const double &data) { dest += std::to_string(data); }
void Writer::write(size_t count, char data) { dest.append(count, data); }
void Writer::writeConstChar(int64_t data)
{
	dest += "'";
	dest.append(1, data);
	dest += "'";
}
void Writer::writeConstString(StringRef data)
{
	dest += "\"";
	dest += toRawString(data);
	dest += "\"";
}

void Writer::writeBefore(StringRef data) { dest.insert(dest.begin(), data.begin(), data.end()); }
void Writer::writeBefore(size_t count, char data) { dest.insert(dest.begin(), count, data); }
void Writer::insertAfter(size_t pos, StringRef data) { dest.insert(pos, data); }

void Writer::reset(Writer &other)
{
	dest.clear();
	indent = other.indent;
}
void Writer::clear() { dest.clear(); }
bool Writer::empty() { return dest.empty(); }

String &Writer::getData() { return dest; }

size_t Writer::getIndent() { return indent; }
} // namespace sc