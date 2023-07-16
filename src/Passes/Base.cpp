#include "Passes/Base.hpp"

namespace sc
{
Pass::Pass(size_t passid, Context &ctx) : passid(passid), ctx(ctx) { ctx.addPass(passid, this); }
Pass::~Pass() { ctx.remPass(passid); }

PassManager::PassManager(Context &ctx) : ctx(ctx) {}
PassManager::~PassManager()
{
	for(auto &p : passes) delete p;
}
bool PassManager::visit(Stmt *&ptree)
{
	for(auto &p : passes) {
		if(!p->visit(ptree, &ptree)) return false;
	}
	return true;
}
} // namespace sc