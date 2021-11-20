/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "CodeGen/Base.hpp"

#include "Parser.hpp"

namespace sc
{
CodeGenDriver::CodeGenDriver(RAIIParser &parser)
	: err(parser.getErrMgr()), ctx(parser.getContext()), parser(parser)
{}
CodeGenDriver::~CodeGenDriver() {}
} // namespace sc