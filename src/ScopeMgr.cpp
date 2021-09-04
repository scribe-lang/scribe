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

#include "ScopeMgr.hpp"

namespace sc
{
TypeValue::TypeValue() : ty(nullptr), val(nullptr) {}
TypeValue::TypeValue(Type *ty, Value *val) : ty(ty), val(val) {}
TypeValue::TypeValue(const TypeValue &tv) : ty(tv.ty), val(tv.val) {}

bool ScopeManager::addVar(const std::string &var, const TypeValue &val, bool global)
{
	if(global) {
		if(globals.find(var) != globals.end()) return false;
		globals[var] = val;
		return true;
	}
	return layers.back().add(var, val);
}
bool ScopeManager::exists(const std::string &var, bool top_only, bool include_globals)
{
	size_t iter = layerlock.size() > 0 ? layerlock.back() : layerlock.size() - 1;
	if(top_only) return !layers[iter].get(var).isNull();
	bool is_done = false;
	while(!is_done && iter >= 0) {
		if(iter == 0) is_done = true;
		if(layers[iter].exists(var)) return true;
		--iter;
	}
	return include_globals ? globals.find(var) != globals.end() : false;
}
TypeValue ScopeManager::get(const std::string &var, bool top_only, bool include_globals)
{
	size_t iter = layerlock.size() > 0 ? layerlock.back() : layerlock.size() - 1;
	if(top_only) return layers[iter].get(var);
	bool is_done = false;
	while(!is_done && iter >= 0) {
		if(iter == 0) is_done = true;
		TypeValue res = layers[iter].get(var);
		if(!res.isNull()) return res;
		--iter;
	}
	auto gres = globals.find(var);
	if(gres != globals.end()) return gres->second;
	return TypeValue(nullptr, nullptr);
}
} // namespace sc