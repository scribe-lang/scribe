#pragma once

#include <filesystem>

#include "Core.hpp"

#define MAX_PATH_CHARS 4096

namespace sc
{
namespace fs
{
inline bool exists(StringRef loc) { return std::filesystem::exists(loc); }
inline bool mkdir(StringRef dir) { return exists(dir) || std::filesystem::create_directories(dir); }
inline String absPath(StringRef loc) { return std::filesystem::absolute(loc).string(); }
inline String getCWD() { return std::filesystem::current_path().string(); }
inline void setCWD(StringRef path) { return std::filesystem::current_path(path); }
inline String parentDir(StringRef path)
{
	return std::filesystem::path(path).parent_path().string();
}
inline String baseName(StringRef path) { return std::filesystem::path(path).filename().string(); }

int getLastTotalLines();

bool read(const String &file, String &data);

String homeDir();
} // namespace fs
} // namespace sc
