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