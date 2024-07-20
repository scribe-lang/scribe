#include "AST/DeferStack.hpp"

namespace sc::AST
{
DeferStack::DeferStack() {}
Vector<Stmt *> DeferStack::getAllStmts()
{
	Vector<Stmt *> res;
	for(auto stackit = stack.back().rbegin(); stackit != stack.back().rend(); ++stackit) {
		auto &frame = *stackit;
		for(auto stmtit = frame.rbegin(); stmtit != frame.rend(); ++stmtit) {
			auto &stmt = *stmtit;
			res.push_back(stmt);
		}
	}
	return res;
}
} // namespace sc::AST