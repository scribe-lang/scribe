#pragma once

#include "Stmts.hpp"

namespace sc::AST
{
class DeferStack
{
	// hierarchy:
	// global/func -> blocks -> statements
	Vector<Vector<Vector<AST::Stmt *>>> stack;

public:
	DeferStack();
	Vector<AST::Stmt *> getAllStmts(Context &c);

	inline void pushFunc() { stack.push_back({}); }
	inline void popFunc() { stack.pop_back(); }
	inline void pushFrame() { stack.back().push_back({}); }
	inline void popFrame() { stack.back().pop_back(); }
	inline void addStmt(AST::Stmt *s) { stack.back().back().push_back(s); }
	inline Span<AST::Stmt *> getTopStmts(Context &c) { return stack.back().back(); }
};
} // namespace sc::AST
