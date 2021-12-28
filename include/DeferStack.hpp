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

#ifndef DEFER_STACK_HPP
#define DEFER_STACK_HPP

#include <string> // for size_t
#include <unordered_map>
#include <vector>

#include "Parser/Stmts.hpp"

namespace sc
{
class DeferStack
{
	// hierarchy:
	// global/func -> blocks -> statements
	std::vector<std::vector<std::vector<Stmt *>>> stack;

public:
	DeferStack();
	inline void pushFunc()
	{
		stack.push_back({});
	}
	inline void popFunc()
	{
		stack.pop_back();
	}
	inline void pushFrame()
	{
		stack.back().push_back({});
	}
	inline void popFrame()
	{
		stack.back().pop_back();
	}
	inline void addStmt(Stmt *s)
	{
		stack.back().back().push_back(s);
	}
	std::vector<Stmt *> getTopStmts(Context &c);
	std::vector<Stmt *> getAllStmts(Context &c);
};
} // namespace sc

#endif // DEFER_STACK_HPP