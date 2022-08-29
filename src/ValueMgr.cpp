/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "ValueMgr.hpp"

#include "PrimitiveTypeFuncs.hpp"

namespace sc
{
bool LayerStack::exists(StringRef name, bool top_only)
{
	size_t i     = layers.size() - 1;
	bool is_done = false;
	while(!is_done && i >= 0) {
		if(i == 0) is_done = true;
		if(layers[i].exists(name)) return true;
		if(top_only) break;
		--i;
	}
	return false;
}
Type *LayerStack::getTy(StringRef name, bool top_only)
{
	size_t i     = layers.size() - 1;
	bool is_done = false;
	while(!is_done && i >= 0) {
		if(i == 0) is_done = true;
		Type *res = layers[i].getTy(name);
		if(res) return res;
		if(top_only) break;
		--i;
	}
	return nullptr;
}
Value *LayerStack::getVal(StringRef name, bool top_only)
{
	size_t i     = layers.size() - 1;
	bool is_done = false;
	while(!is_done && i >= 0) {
		if(i == 0) is_done = true;
		Value *res = layers[i].getVal(name);
		if(res) return res;
		if(top_only) break;
		--i;
	}
	return nullptr;
}
StmtVar *LayerStack::getDecl(StringRef name, bool top_only)
{
	size_t i     = layers.size() - 1;
	bool is_done = false;
	while(!is_done && i >= 0) {
		if(i == 0) is_done = true;
		StmtVar *res = layers[i].getDecl(name);
		if(res) return res;
		if(top_only) break;
		--i;
	}
	return nullptr;
}
VarDecl *LayerStack::getAll(StringRef name, bool top_only)
{
	size_t i     = layers.size() - 1;
	bool is_done = false;
	while(!is_done && i >= 0) {
		if(i == 0) is_done = true;
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
	if(res || top_only || !include_globals) return res;
	return globals.find(var) != globals.end();
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
	if(!funcstack.empty()) {
		Type *res = funcstack.back().getTy(var, top_only);
		if(res || top_only) return res;
	}
	Type *res = layers.getTy(var, top_only);
	if(res || top_only || !include_globals) return res;
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second.ty;
	return nullptr;
}
Value *ValueManager::getVal(StringRef var, bool top_only, bool include_globals)
{
	if(!funcstack.empty()) {
		Value *res = funcstack.back().getVal(var, top_only);
		if(res || top_only) return res;
	}
	Value *res = layers.getVal(var, top_only);
	if(res || top_only || !include_globals) return res;
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second.val;
	return nullptr;
}
StmtVar *ValueManager::getDecl(StringRef var, bool top_only, bool include_globals)
{
	if(!funcstack.empty()) {
		StmtVar *res = funcstack.back().getDecl(var, top_only);
		if(res || top_only) return res;
	}
	StmtVar *res = layers.getDecl(var, top_only);
	if(res || top_only || !include_globals) return res;
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second.decl;
	return nullptr;
}
VarDecl *ValueManager::getAll(StringRef var, bool top_only, bool include_globals)
{
	if(!funcstack.empty()) {
		VarDecl *res = funcstack.back().getAll(var, top_only);
		if(res || top_only) return res;
	}
	VarDecl *res = layers.getAll(var, top_only);
	if(res || top_only || !include_globals) return res;
	auto gres = globals.find(var);
	if(gres != globals.end()) return &gres->second;
	return nullptr;
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