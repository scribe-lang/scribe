#pragma once

#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <forward_list>
#include <initializer_list>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sc
{

// the primitives have lower case name
using u8 = unsigned char;

using String	= std::string;
using Nullptr	= std::nullptr_t;
using StringRef = std::string_view;

template<typename T> using Set		   = std::unordered_set<T>;
template<typename T> using List		   = std::forward_list<T>; // singly linked list
template<typename T> using Span		   = std::span<T>;
template<typename T> using Vector	   = std::vector<T>;
template<typename T> using InitList	   = std::initializer_list<T>;
template<typename K, typename V> using Map = std::unordered_map<K, V>;

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
