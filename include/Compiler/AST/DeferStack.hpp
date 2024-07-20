#pragma once

#include "Stmts.hpp"

namespace sc::ast
{
class DeferStack
{
	// hierarchy:
	// global/func -> blocks -> statements
	Vector<Vector<Vector<ast::Stmt *>>> stack;

public:
	DeferStack();
	Vector<ast::Stmt *> getAllStmts();

	inline void pushFunc() { stack.push_back({}); }
	inline void popFunc() { stack.pop_back(); }
	inline void pushFrame() { stack.back().push_back({}); }
	inline void popFrame() { stack.back().pop_back(); }
	inline void addStmt(ast::Stmt *s) { stack.back().back().push_back(s); }
	inline Span<ast::Stmt *> getTopStmts() { return stack.back().back(); }
};
} // namespace sc::ast
