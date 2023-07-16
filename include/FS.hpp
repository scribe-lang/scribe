#pragma once

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
