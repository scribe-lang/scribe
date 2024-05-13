#include "Env.hpp"

#include "FS.hpp"
#include "Utils.hpp"

#if defined(OS_APPLE)
	#include <mach-o/dyld.h> // for _NSGetExecutablePath()
#elif defined(OS_FREEBSD)
	#include <sys/sysctl.h> // for sysctl()
	#include <sys/types.h>
#elif defined(OS_WINDOWS)
	#include <Windows.h>
#else
	#include <unistd.h> // for readlink()
#endif

namespace sc
{
namespace env
{
const char *getPathDelim()
{
#if defined(OS_WINDOWS)
	return ";";
#else
	return ":";
#endif
}

String get(const String &key)
{
	const char *env = getenv(key.c_str());
	return env == NULL ? "" : env;
}

String getProcPath()
{
	char path[MAX_PATH_CHARS];
	memset(path, 0, MAX_PATH_CHARS);
#if defined(OS_LINUX) || defined(OS_ANDROID)
	(void)readlink("/proc/self/exe", path, MAX_PATH_CHARS);
#elif defined(OS_WINDOWS)
	GetModuleFileName(nullptr, path, MAX_PATH_CHARS);
#elif defined(OS_FREEBSD)
	int mib[4];
	mib[0]	  = CTL_KERN;
	mib[1]	  = KERN_PROC;
	mib[2]	  = KERN_PROC_PATHNAME;
	mib[3]	  = -1;
	size_t sz = MAX_PATH_CHARS;
	sysctl(mib, 4, path, &sz, NULL, 0);
#elif defined(OS_NETBSD)
	readlink("/proc/curproc/exe", path, MAX_PATH_CHARS);
#elif defined(OS_OPENBSD) || defined(OS_DRAGONFLYBSD)
	readlink("/proc/curproc/file", path, MAX_PATH_CHARS);
#elif defined(OS_APPLE)
	uint32_t sz = MAX_PATH_CHARS;
	_NSGetExecutablePath(path, &sz);
#endif
	return path;
}

String getExeFromPath(StringRef exe)
{
	String path = get("PATH");
	if(path.empty()) return "";

	Vector<StringRef> paths = stringDelim(path, getPathDelim());

	String tmp;
	for(auto &p : paths) {
		tmp += p;
		tmp += "/";
		tmp += exe;
		if(fs::exists(tmp)) break;
		tmp.clear();
	}
	return tmp;
}

int exec(const String &cmd)
{
	int res = std::system(cmd.c_str());
#if !defined(OS_WINDOWS)
	res = WEXITSTATUS(res);
#endif
	return res;
}
} // namespace env
} // namespace sc