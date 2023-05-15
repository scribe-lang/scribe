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

#include <filesystem>

#include "Core.hpp"

#define MAX_PATH_CHARS 4096

namespace sc
{
namespace fs
{
inline bool exists(const String &loc) { return std::filesystem::exists(loc); }
inline bool mkdir(const String &dir)
{
	return exists(dir) || std::filesystem::create_directories(dir);
}
inline String absPath(const String &loc) { return std::filesystem::absolute(loc).string(); }
inline String getCWD() { return std::filesystem::current_path().string(); }
inline void setCWD(const String &path) { return std::filesystem::current_path(path); }
inline String parentDir(const String &path)
{
	return std::filesystem::path(path).parent_path().string();
}
inline String baseName(const String &path)
{
	return std::filesystem::path(path).filename().string();
}

int getLastTotalLines();

bool read(const String &file, String &data);

String home();
} // namespace fs
} // namespace sc

#endif // FS_HPP