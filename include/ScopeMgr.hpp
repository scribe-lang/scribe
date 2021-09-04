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

#ifndef SCOPEMGR_HPP
#define SCOPEMGR_HPP

#include <unordered_map>

#include "Types.hpp"
#include "Values.hpp"

namespace sc
{
class TypeValue
{
	Type *ty;
	Value *val;

public:
	TypeValue();
	TypeValue(Type *ty, Value *val);
	TypeValue(const TypeValue &tv);

	inline void setType(Type *t)
	{
		ty = t;
	}
	inline void setVal(Value *v)
	{
		val = v;
	}

	inline Type *getType()
	{
		return ty;
	}
	inline Value *getValue()
	{
		return val;
	}

	inline bool isNull() const
	{
		return !ty && !val;
	}
};
class Layer
{
	std::unordered_map<std::string, TypeValue> items;

public:
	inline bool add(const std::string &name, const TypeValue &val)
	{
		if(exists(name)) return false;
		items.insert({name, val});
		return true;
	}
	inline bool exists(const std::string &name)
	{
		return items.find(name) != items.end();
	}
	inline TypeValue get(const std::string &name)
	{
		return items.find(name) == items.end() ? TypeValue(nullptr, nullptr) : items[name];
	}
	inline std::unordered_map<std::string, TypeValue> &getItems()
	{
		return items;
	}
};
class ScopeManager
{
	std::unordered_map<std::string, TypeValue> globals;
	std::vector<Layer> layers;
	std::vector<size_t> layerlock;

public:
	inline void pushLayer()
	{
		layers.push_back({});
	}
	inline void popLayer()
	{
		layers.pop_back();
	}
	inline size_t getCurrentLayerIndex()
	{
		return layers.size() - 1;
	}
	inline void lockLayerAfter(const size_t &idx)
	{
		layerlock.push_back(idx);
	}
	inline void unlockScope()
	{
		layerlock.pop_back();
	}
	bool addVar(const std::string &var, const TypeValue &val, bool global = false);
	bool exists(const std::string &var, bool top_only, bool include_globals);
	TypeValue get(const std::string &var, bool top_only, bool include_globals);
};
} // namespace sc

#endif // SCOPEMGR_HPP