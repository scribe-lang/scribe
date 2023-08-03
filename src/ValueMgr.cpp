#include "ValueMgr.hpp"

#include "PrimitiveTypeFuncs.hpp"

namespace sc
{
bool LayerStack::exists(StringRef name, bool top_only)
{
	ssize_t i = layers.size() - 1;
	while(i >= 0) {
		if(layers[i].exists(name)) return true;
		if(top_only) break;
		--i;
	}
	return false;
}
VarDecl *LayerStack::getAll(StringRef name, bool top_only)
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

Function::Function(FuncTy *ty) : fty(ty) {}

ValueManager::ValueManager(Context &c) { AddPrimitiveFuncs(c, *this); }
bool ValueManager::addVar(StringRef var, Type *ty, Value *val, StmtVar *decl, bool global)
{
	if(global) {
		if(globals.find(var) != globals.end()) return false;
		globals[var] = {ty, val, decl};
		return true;
	}
	if(!funcstack.empty()) return funcstack.back().add(var, ty, val, decl);
	return layers.add(var, ty, val, decl);
}
bool ValueManager::addTypeFn(Type *ty, StringRef name, FuncVal *fn)
{
	return addTypeFn(ty->getID(), name, fn);
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
bool ValueManager::exists(StringRef var, bool top_only, bool include_globals)
{
	if(!funcstack.empty()) {
		bool res = funcstack.back().exists(var, top_only);
		if(res || top_only) return res;
	}
	bool res = layers.exists(var, top_only);
	if(!res && include_globals) return globals.find(var) != globals.end();
	return res;
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
Type *ValueManager::getTy(StringRef var, bool top_only, bool include_globals)
{
	VarDecl *decl = getAll(var, top_only, include_globals);
	return decl ? decl->ty : nullptr;
}
Value *ValueManager::getVal(StringRef var, bool top_only, bool include_globals)
{
	VarDecl *decl = getAll(var, top_only, include_globals);
	return decl ? decl->val : nullptr;
}
StmtVar *ValueManager::getDecl(StringRef var, bool top_only, bool include_globals)
{
	VarDecl *decl = getAll(var, top_only, include_globals);
	return decl ? decl->decl : nullptr;
}
VarDecl *ValueManager::getAll(StringRef var, bool top_only, bool include_globals)
{
	if(!funcstack.empty()) {
		VarDecl *res = funcstack.back().getAll(var, top_only);
		if(res || top_only) return res;
	}
	VarDecl *res = layers.getAll(var, top_only);
	if(!res && include_globals) {
		auto gres = globals.find(var);
		if(gres != globals.end()) return &gres->second;
	}
	return res;
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