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

#ifndef TREE_IO_HPP
#define TREE_IO_HPP

#include "Core.hpp"

namespace sc
{
namespace tio
{
void taba(const bool show);
void tabr(const size_t num = 1);
void print(const bool has_next, InitList<StringRef> data);
void printf(InitList<StringRef> data);
} // namespace tio
} // namespace sc

#endif // TREE_IO_HPP