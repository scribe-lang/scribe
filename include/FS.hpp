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

#ifndef FS_HPP
#define FS_HPP

#include "Core.hpp"

#define MAX_PATH_CHARS 4096

namespace sc
{
namespace fs
{
bool exists(const String &loc);

bool read(const String &file, String &data);

String absPath(const String &loc);

String getCWD();

bool setCWD(const String &path);

String parentDir(const String &path);

String home();
} // namespace fs
} // namespace sc

#endif // FS_HPP