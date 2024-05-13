#pragma once

#include "Core.hpp"

namespace sc
{
namespace env
{
const char *getPathDelim();

String get(const String &key);

String getProcPath();

String getExeFromPath(StringRef exe);

int exec(const String &cmd);
} // namespace env
} // namespace sc
