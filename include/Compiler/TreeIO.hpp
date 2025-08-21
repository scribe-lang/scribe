#pragma once

#include "Core.hpp"
#include "Utils.hpp"

namespace sc::tio
{

extern Vector<bool> _tabs;

void applyTab(bool has_next);

inline void tabAdd(bool show) { _tabs.push_back(show); }
inline void tabRem(size_t num = 1)
{
	while(num-- > 0) _tabs.pop_back();
}
inline void printStr(OStream &os, const String &data) { os << data; }
template<typename... Args> void printWithoutTab(OStream &os, Args &&...args)
{
	String res;
	utils::appendToString(res, std::forward<Args>(args)...);
	printStr(os, res);
}
template<typename... Args> void print(OStream &os, bool has_next, Args &&...args)
{
	applyTab(has_next);
	printWithoutTab(os, std::forward<Args>(args)...);
}

} // namespace sc::tio
