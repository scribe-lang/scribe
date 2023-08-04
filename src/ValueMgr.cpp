#include "ValueMgr.hpp"

#include "PrimitiveTypeFuncs.hpp"

namespace sc
{
Function::Function(FuncTy *ty) : fty(ty) {}
bool Function::exists(StringRef name, bool top_only)
{
	ssize_t i = layers.size() - 1;
	while(i >= 0) {
		if(layers[i].exists(name)) return true;
		if(top_only) break;
		--i;
	}
	return false;
}
VarDecl *Function::getAll(StringRef name, bool top_only)
{
	ssize_t i = layers.size() - 1;
	while(i >= 0) {
		VarDecl *res = layers[i].getAll(name);
		if(res) return res;
		if(top_only) break;
		--i;
	}
	return nullptr;
}

ValueManager::ValueManager(Context &c) { AddPrimitiveFuncs(c, *this); }
bool ValueManager::addVar(StringRef var, Type *ty, Value *val, StmtVar *decl, bool global)
{
	if(!funcstack.empty()) return funcstack.back().add(var, ty, val, decl);
	return globals.add(var, ty, val, decl);
}
bool ValueManager::addTypeFn(uint32_t id, StringRef name, FuncVal *fn)
{
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	if(funcmap.find(name) != funcmap.end()) return false;
	funcmap[name] = fn;
	return true;
}
bool ValueManager::exists(StringRef var, bool top_only)
{
	if(!funcstack.empty()) {
		bool res = funcstack.back().exists(var, top_only);
		if(res || top_only) return res;
	}
	return globals.exists(var);
}
bool ValueManager::existsTypeFn(Type *ty, StringRef name)
{
	uint32_t id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	return funcmap.find(name) != funcmap.end();
}
Type *ValueManager::getTy(StringRef var, bool top_only)
{
	VarDecl *decl = getAll(var, top_only);
	return decl ? decl->ty : nullptr;
}
Value *ValueManager::getVal(StringRef var, bool top_only)
{
	VarDecl *decl = getAll(var, top_only);
	return decl ? decl->val : nullptr;
}
StmtVar *ValueManager::getDecl(StringRef var, bool top_only)
{
	VarDecl *decl = getAll(var, top_only);
	return decl ? decl->decl : nullptr;
}
VarDecl *ValueManager::getAll(StringRef var, bool top_only)
{
	if(!funcstack.empty()) {
		VarDecl *res = funcstack.back().getAll(var, top_only);
		if(res || top_only) return res;
	}
	return globals.getAll(var);
}
FuncVal *ValueManager::getTyFn(Type *ty, StringRef name)
{
	uint32_t id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	auto found    = funcmap.find(name);
	if(found == funcmap.end()) return nullptr;
	return found->second;
}
} // namespace sc