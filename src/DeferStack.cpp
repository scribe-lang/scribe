/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

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
DeferStack::DeferStack() {}
Vector<Stmt *> DeferStack::getTopStmts(Context &c)
{
	Vector<Stmt *> res = stack.back().back();
	for(auto &s : res) s = s->clone(c);
	return res;
}
Vector<Stmt *> DeferStack::getAllStmts(Context &c)
{
	Vector<Stmt *> res;
	for(auto stackit = stack.back().rbegin(); stackit != stack.back().rend(); ++stackit) {
		auto &frame = *stackit;
		for(auto stmtit = frame.rbegin(); stmtit != frame.rend(); ++stmtit) {
			auto &stmt = *stmtit;
			res.push_back(stmt);
		}
	}
	for(auto &s : res) s = s->clone(c);
	return res;
}
} // namespace sc