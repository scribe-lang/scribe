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

#include "ValueMgr.hpp"

#include "PrimitiveTypeFuncs.hpp"

namespace sc
{
ValueManager::ValueManager(Context &c)
{
	AddPrimitiveFuncs(c, *this);
}
bool ValueManager::addVar(const std::string &var, const uint64_t &vid, StmtVar *decl, bool global)
{
	if(global) {
		if(globals.find(var) != globals.end()) return false;
		globals[var] = {vid, decl};
		return true;
	}
	return layers.back().add(var, vid, decl);
}
bool ValueManager::addTypeFn(Type *ty, const std::string &name, const uint64_t &fn)
{
	return addTypeFn(ty->getID(), name, fn);
}
bool ValueManager::addTypeFn(const uint64_t &id, const std::string &name, const uint64_t &fn)
{
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	if(funcmap.find(name) != funcmap.end()) return false;
	funcmap[name] = fn;
	return true;
}
bool ValueManager::exists(const std::string &var, bool top_only, bool include_globals)
{
	size_t iter   = layers.size() - 1;
	size_t locked = layerlock.size() > 0 ? layerlock.back() : 0;
	bool is_done  = false;
	while(!is_done && iter >= 0) {
		if(iter == 0) is_done = true;
		if(locked && iter && iter <= locked) {
			--iter;
			if(top_only) return false;
			continue;
		}
		if(layers[iter].exists(var)) return true;
		if(top_only) return false;
		--iter;
	}
	return include_globals ? globals.find(var) != globals.end() : false;
}
bool ValueManager::existsTypeFn(Type *ty, const std::string &name)
{
	const uint64_t &id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	return funcmap.find(name) != funcmap.end();
}
uint64_t ValueManager::getVar(const std::string &var, bool top_only, bool include_globals)
{
	size_t iter   = layers.size() - 1;
	size_t locked = layerlock.size() > 0 ? layerlock.back() : 0;
	bool is_done  = false;
	while(!is_done && iter >= 0) {
		if(iter == 0) is_done = true;
		if(locked && iter && iter <= locked) {
			--iter;
			if(top_only) return 0;
			continue;
		}
		uint64_t res = layers[iter].getVal(var);
		if(res) return res;
		if(top_only) return 0;
		--iter;
	}
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second.valueid;
	return 0;
}
StmtVar *ValueManager::getDecl(const std::string &var, bool top_only, bool include_globals)
{
	size_t iter   = layers.size() - 1;
	size_t locked = layerlock.size() > 0 ? layerlock.back() : 0;
	bool is_done  = false;
	while(!is_done && iter >= 0) {
		if(iter == 0) is_done = true;
		if(locked && iter && iter <= locked) {
			--iter;
			if(top_only) return nullptr;
			continue;
		}
		StmtVar *res = layers[iter].getDecl(var);
		if(res) return res;
		if(top_only) return nullptr;
		--iter;
	}
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second.decl;
	return nullptr;
}
uint64_t ValueManager::getTypeFn(Type *ty, const std::string &name)
{
	const uint64_t &id = ty->getID();
	if(typefuncs.find(id) == typefuncs.end()) {
		typefuncs[id] = {};
	}
	auto &funcmap = typefuncs[id];
	auto found    = funcmap.find(name);
	if(found == funcmap.end()) return 0;
	return found->second;
}
} // namespace sc