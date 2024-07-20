#pragma once

#include "Core.hpp"
#include "Utils.hpp"

namespace sc
{
namespace tio
{
extern Vector<bool> _tabs;

void applyTab(bool has_next);

inline void tabAdd(bool show) { _tabs.push_back(show); }
inline void tabRem(size_t num = 1)
{
	while(num-- > 0) _tabs.pop_back();
}
inline void printStr(const String &data) { std::cout << data; }
template<typename... Args> void printWithoutTab(Args &&...args)
{
	String res;
	utils::appendToString(res, std::forward<Args>(args)...);
	printStr(res);
}
template<typename... Args> void print(bool has_next, Args &&...args)
{
	applyTab(has_next);
	printWithoutTab(std::forward<Args>(args)...);
}
} // namespace tio
} // namespace sc
