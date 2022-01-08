/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "TreeIO.hpp"

#include <iostream>

namespace sc
{
namespace tio
{
static Vector<bool> &_tab()
{
	static Vector<bool> tabs;
	return tabs;
}

static void _tab_apply(const bool has_next)
{
	for(size_t i = 0; i < _tab().size(); ++i) {
		if(i == _tab().size() - 1) {
			std::cout << (has_next ? " ├─" : " └─");
		} else {
			std::cout << (_tab()[i] ? " │" : "  ");
		}
	}
}

void taba(const bool show)
{
	_tab().push_back(show);
}

void tabr(const size_t num)
{
	if(num > _tab().size()) return;
	for(size_t i = 0; i < num; ++i) _tab().pop_back();
}

void print(const bool has_next, InitList<StringRef> data)
{
	_tab_apply(has_next);
	for(auto &d : data) std::cout << d;
}
void printf(InitList<StringRef> data)
{
	for(auto &d : data) std::cout << d;
}
} // namespace tio
} // namespace sc
