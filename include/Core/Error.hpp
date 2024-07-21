#pragma once

#include "Utils.hpp"

namespace sc
{

class ModuleLoc
{
	size_t moduleId; // an index
	size_t offset;	 // pretty much of type std::streamoff

public:
	ModuleLoc();
	ModuleLoc(StringRef filePath, size_t offset);
	ModuleLoc(size_t moduleId, size_t offset);

	static size_t getOrAddModuleIdForPath(StringRef filePath);
	static const String &getModuleNameFromId(size_t id);

	inline const String &getModuleName() const { return getModuleNameFromId(moduleId); }
	inline size_t getModuleId() const { return moduleId; }
	inline size_t getOffset() const { return offset; }
	inline bool isValid() const { return moduleId != -1; }
};

namespace err
{

extern size_t max_errs;

inline void setMaxErrs(size_t max_err) { max_errs = max_err; }

void outCommonStr(const ModuleLoc *loc, bool iswarn, bool withloc, const String &e);

template<typename... Args>
void outCommon(const ModuleLoc *loc, bool iswarn, bool withloc, Args &&...args)
{
	String res;
	utils::appendToString(res, std::forward<Args>(args)...);
	outCommonStr(loc, iswarn, withloc, res);
}

template<typename... Args> void out(ModuleLoc loc, Args &&...args)
{
	outCommon(&loc, false, true, std::forward<Args>(args)...);
}

// equivalent to out(), but for warnings
template<typename... Args> void outw(ModuleLoc loc, Args &&...args)
{
	outCommon(&loc, true, true, std::forward<Args>(args)...);
}

template<typename... Args> void out(Nullptr, Args &&...args)
{
	outCommon(nullptr, false, false, std::forward<Args>(args)...);
}
template<typename... Args> void outw(Nullptr, Args &&...args)
{
	outCommon(nullptr, true, false, std::forward<Args>(args)...);
}

} // namespace err
} // namespace sc
