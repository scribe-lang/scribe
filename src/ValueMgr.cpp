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
uint64_t LayerStack::getVal(StringRef name, bool top_only)
{
	size_t i     = layers.size() - 1;
	bool is_done = false;
	while(!is_done && i >= 0) {
		if(i == 0) is_done = true;
		uint64_t res = layers[i].getVal(name);
		if(res) return res;
		if(top_only) break;
		--i;
	}
	return 0;
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

Function::Function(FuncTy *ty) : fty(ty) {}

ValueManager::ValueManager(Context &c)
{
	AddPrimitiveFuncs(c, *this);
}
bool ValueManager::addVar(StringRef var, uint64_t vid, StmtVar *decl, bool global)
{
	if(global) {
		if(globals.find(var) != globals.end()) return false;
		globals[var] = {vid, decl};
		return true;
	}
	if(!funcstack.empty()) return funcstack.back().add(var, vid, decl);
	return layers.add(var, vid, decl);
}
bool ValueManager::addTypeFn(Type *ty, StringRef name, uint64_t fn)
{
	return addTypeFn(ty->getID(), name, fn);
}
bool ValueManager::addTypeFn(uint64_t id, StringRef name, uint64_t fn)
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
	uint64_t id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	return funcmap.find(name) != funcmap.end();
}
uint64_t ValueManager::getVar(StringRef var, bool top_only, bool include_globals)
{
	if(!funcstack.empty()) {
		uint64_t res = funcstack.back().getVal(var, top_only);
		if(res || top_only) return res;
	}
	uint64_t res = layers.getVal(var, top_only);
	if(res || top_only || !include_globals) return res;
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second.valueid;
	return 0;
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
uint64_t ValueManager::getTypeFn(Type *ty, StringRef name)
{
	uint64_t id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	auto found    = funcmap.find(name);
	if(found == funcmap.end()) return 0;
	return found->second;
}
} // namespace sc