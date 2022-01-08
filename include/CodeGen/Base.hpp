/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef CODEGEN_BASE_HPP
#define CODEGEN_BASE_HPP

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

#endif // CODEGEN_BASE_HPP