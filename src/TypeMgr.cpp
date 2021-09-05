/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "TypeMgr.hpp"

namespace sc
{
bool TypeManager::addVar(const std::string &var, Type *val, Stmt *decl, bool global)
{
	if(global) {
		if(globals.find(var) != globals.end()) return false;
		globals[var] = {val, decl};
		return true;
	}
	return layers.back().add(var, val, decl);
}
bool TypeManager::addTypeFn(Type *ty, const std::string &name, FuncTy *fn)
{
	const uint64_t &id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	if(funcmap.find(name) != funcmap.end()) return false;
	funcmap[name] = fn;
	return true;
}
bool TypeManager::exists(const std::string &var, bool top_only, bool include_globals)
{
	size_t iter = layerlock.size() > 0 ? layerlock.back() : layerlock.size() - 1;
	if(top_only) return layers[iter].getTy(var);
	bool is_done = false;
	while(!is_done && iter >= 0) {
		if(iter == 0) is_done = true;
		if(layers[iter].exists(var)) return true;
		--iter;
	}
	return include_globals ? globals.find(var) != globals.end() : false;
}
bool TypeManager::existsTypeFn(Type *ty, const std::string &name)
{
	const uint64_t &id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	return funcmap.find(name) != funcmap.end();
}
Type *TypeManager::getTy(const std::string &var, bool top_only, bool include_globals)
{
	size_t iter = layerlock.size() > 0 ? layerlock.back() : layerlock.size() - 1;
	if(top_only) return layers[iter].getTy(var);
	bool is_done = false;
	while(!is_done && iter >= 0) {
		if(iter == 0) is_done = true;
		Type *res = layers[iter].getTy(var);
		if(res) return res;
		--iter;
	}
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second.ty;
	return nullptr;
}
Stmt *TypeManager::getDecl(const std::string &var, bool top_only, bool include_globals)
{
	size_t iter = layerlock.size() > 0 ? layerlock.back() : layerlock.size() - 1;
	if(top_only) return layers[iter].getDecl(var);
	bool is_done = false;
	while(!is_done && iter >= 0) {
		if(iter == 0) is_done = true;
		Stmt *res = layers[iter].getDecl(var);
		if(res) return res;
		--iter;
	}
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second.decl;
	return nullptr;
}
FuncTy *TypeManager::getTypeFn(Type *ty, const std::string &name)
{
	const uint64_t &id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	auto found    = funcmap.find(name);
	if(found == funcmap.end()) return nullptr;
	return found->second;
}
} // namespace sc