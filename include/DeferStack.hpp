#pragma once

#include <unordered_map>
#include <vector>

#include "Parser/Stmts.hpp"

namespace sc
{
class DeferStack
{
	// hierarchy:
	// global/func -> blocks -> statements
	Vector<Vector<Vector<Stmt *>>> stack;

public:
	DeferStack();
	inline void pushFunc() { stack.push_back({}); }
	inline void popFunc() { stack.pop_back(); }
	inline void pushFrame() { stack.back().push_back({}); }
	inline void popFrame() { stack.back().pop_back(); }
	inline void addStmt(Stmt *s) { stack.back().back().push_back(s); }
	Vector<Stmt *> getTopStmts(Context &c);
	Vector<Stmt *> getAllStmts(Context &c);
};
} // namespace sc
