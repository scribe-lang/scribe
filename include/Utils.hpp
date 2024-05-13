#pragma once

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
	void set(T *dat) { data = dat; }
	void unset() { data = nullptr; }
};

inline bool startsWith(StringRef src, StringRef term) { return src.rfind(term, 0) == 0; }

// Also trims the spaces for each split
Vector<StringRef> stringDelim(StringRef str, StringRef delim);

// Convert special characters in string (\n, \t, ...) to raw (\\n, \\t, ...)
// and vice versa
String toRawString(StringRef data);
String fromRawString(StringRef data);

String vecToStr(Span<StringRef> items);
String vecToStr(Span<String> items);

inline void appendToString(String &dest) {}

inline void appendToString(String &dest, bool data) { dest += data ? "(true)" : "(false)"; }
inline void appendToString(String &dest, char data) { dest += data; }
inline void appendToString(String &dest, u8 data) { dest += std::to_string(data); }
inline void appendToString(String &dest, int data) { dest += std::to_string(data); }
inline void appendToString(String &dest, int64_t data) { dest += std::to_string(data); }
inline void appendToString(String &dest, size_t data) { dest += std::to_string(data); }
inline void appendToString(String &dest, float data) { dest += std::to_string(data); }
inline void appendToString(String &dest, double data) { dest += std::to_string(data); }
inline void appendToString(String &dest, const char *data) { dest += data; }
inline void appendToString(String &dest, StringRef data) { dest += data; }
inline void appendToString(String &dest, const String &data) { dest += data; }

template<typename... Args> void appendToString(String &dest, Args... args)
{
	int tmp[] = {(appendToString(dest, args), 0)...};
	static_cast<void>(tmp);
}
template<typename... Args> String toString(Args... args)
{
	String dest;
	appendToString(dest, std::forward<Args>(args)...);
	return dest;
}
} // namespace sc
