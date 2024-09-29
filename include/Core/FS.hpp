#pragma once

#include "Core.hpp"

#if defined(SC_OS_WINDOWS)
#define PATH_DELIM "\\"
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);
ssize_t getline(char **buf, size_t *bufsiz, FILE *fp);
#else
#define PATH_DELIM "/"
#endif

namespace sc::fs
{

StringRef parentDir(StringRef path);

bool exists(StringRef loc);
bool read(const char *file, String &data, int *totalLines = nullptr);
String absPath(const char *loc);
bool setCWD(const char *path);
String getCWD();
StringRef home();

int copy(StringRef src, StringRef dest, std::error_code &ec);
int mkdir(StringRef dir, std::error_code &ec);
int mklink(StringRef src, StringRef dest, std::error_code &ec);
int rename(StringRef from, StringRef to, std::error_code &ec);
int remove(StringRef path, std::error_code &ec);

template<typename... Args> String pathFrom(Args... args)
{
	String res;
	// Fold expression magic :D
	(
	[&] {
		res += args;
		res += PATH_DELIM;
	}(),
	...);
	// Remove path separator from the end.
	if(!res.empty()) res.pop_back();
	return res;
}

} // namespace sc::fs
