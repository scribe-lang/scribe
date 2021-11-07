/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "DeferStack.hpp"

namespace sc
{
DeferStack::~DeferStack() {}
void DeferStack::popFrame()
{
	stack.pop_back();
}
std::vector<Stmt *> DeferStack::getAllStmts()
{
	std::vector<Stmt *> res;
	for(auto stackit = stack.rbegin(); stackit != stack.rend(); ++stackit) {
		auto &frame = *stackit;
		for(auto stmtit = frame.rbegin(); stmtit != frame.rend(); ++stmtit) {
			auto &stmt = *stmtit;
			res.push_back(stmt);
		}
	}
	return res;
}
} // namespace sc