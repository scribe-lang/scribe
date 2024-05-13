#pragma once

#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <forward_list>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace sc
{

// the primitives have lower case name
using i1  = bool;
using i8  = char;
using i16 = short;
using i32 = long;
using i64 = long long;
using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned long;
using u64 = unsigned long long;
using f32 = float;
using f64 = long double;

using String	= std::string;
using Nullptr	= std::nullptr_t;
using StringRef = std::string_view;

struct StringHash
{
	using HashType	     = std::hash<StringRef>;
	using is_transparent = void;

	size_t operator()(const char *str) const { return HashType{}(str); }
	size_t operator()(StringRef str) const { return HashType{}(str); }
	size_t operator()(const String &str) const { return HashType{}(str); }
};

template<typename T> using Set	       = std::unordered_set<T>;
template<typename T> using List	       = std::forward_list<T>; // singly linked list
template<typename T> using Span	       = std::span<T>;
template<typename T> using Vector      = std::vector<T>;
template<typename V> using StringMap   = std::unordered_map<String, V, StringHash, std::equal_to<>>;
template<typename... Ts> using Variant = std::variant<Ts...>;
template<typename K, typename V> using Map = std::unordered_map<K, V>;
template<typename T, size_t N> using Array = std::array<T, N>;

// OS Defs

#if __linux__
	#define OS_LINUX
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	#define OS_WINDOWS
#elif __ANDROID__
	#define OS_ANDROID
#elif __FreeBSD__
	#define OS_FREEBSD
#elif __NetBSD__
	#define OS_NETBSD
#elif __OpenBSD__ || __bsdi__
	#define OS_OPENBSD
#elif __DragonFly__
	#define OS_DRAGONFLYBSD
#elif __APPLE__
	#define OS_APPLE
#endif

} // namespace sc
