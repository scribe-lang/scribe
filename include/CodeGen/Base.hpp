#pragma once

#include "Passes/Base.hpp"

namespace sc
{
class CodeGenDriver
{
protected:
	Context &ctx;
	RAIIParser &parser;

public:
	CodeGenDriver(RAIIParser &parser);
	virtual ~CodeGenDriver();

	virtual bool compile(StringRef outfile) = 0;
};
} // namespace sc
