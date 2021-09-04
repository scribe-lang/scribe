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

#ifndef ENV_HPP
#define ENV_HPP

#include <string>

namespace sc
{
namespace env
{
std::string get(const std::string &key);

std::string getProcPath();
} // namespace env
} // namespace sc

#endif // ENV_HPP