#include "AST/Passes/Base.hpp"

namespace sc::ast
{
Pass::Pass(size_t passid) : passid(passid) {}
Pass::~Pass() {}

PassManager::PassManager() {}
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
} // namespace sc::ast