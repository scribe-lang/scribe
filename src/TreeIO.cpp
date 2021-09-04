/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "TreeIO.hpp"

#include <cstdarg>
#include <string>
#include <vector>

namespace sc
{
namespace tio
{
static std::vector<bool> &_tab()
{
	static std::vector<bool> tabs;
	return tabs;
}

static void _tab_apply(const bool has_next)
{
	for(size_t i = 0; i < _tab().size(); ++i) {
		if(i == _tab().size() - 1) {
			fprintf(stdout, has_next ? " ├─" : " └─");
		} else {
			fprintf(stdout, _tab()[i] ? " │" : "  ");
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

void print(const bool has_next, const char *fmt, ...)
{
	_tab_apply(has_next);
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}
void printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}
} // namespace tio
} // namespace sc
