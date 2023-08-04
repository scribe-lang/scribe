#pragma once

#include "Core.hpp"
#include "Utils.hpp"

namespace sc
{
class Module;

class ModuleLoc
{
	Module *mod;
	size_t line;
	size_t col;

public:
	ModuleLoc(Module *mod, size_t line, size_t col);

	String getLocStr() const;
	inline Module *getMod() const { return mod; }
	inline size_t getLine() const { return line; }
	inline size_t getCol() const { return col; }
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
	appendToString(res, std::forward<Args>(args)...);
	outCommonStr(loc, iswarn, withloc, res);
}

template<typename... Args> void out(const ModuleLoc *loc, Args &&...args)
{
	outCommon(loc, false, loc, std::forward<Args>(args)...);
}
template<typename... Args> void out(ModuleLoc &&loc, Args &&...args)
{
	outCommon(&loc, false, true, std::forward<Args>(args)...);
}

// equivalent to out(), but for warnings
template<typename... Args> void outw(const ModuleLoc *loc, Args &&...args)
{
	outCommon(loc, true, loc, std::forward<Args>(args)...);
}
template<typename... Args> void outw(ModuleLoc &&loc, Args &&...args)
{
	outCommon(&loc, true, true, std::forward<Args>(args)...);
}

template<typename... Args> void out(Nullptr, Args &&...args)
{
	out(static_cast<const ModuleLoc *>(nullptr), std::forward<Args>(args)...);
}
template<typename... Args> void outw(Nullptr, Args &&...args)
{
	outw(static_cast<const ModuleLoc *>(nullptr), std::forward<Args>(args)...);
}

} // namespace err
} // namespace sc
