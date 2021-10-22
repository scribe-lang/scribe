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

#ifndef TYPEMGR_HPP
#define TYPEMGR_HPP

#include <unordered_map>

#include "Parser/Stmts.hpp"

namespace sc
{
struct TypeDecl
{
	Type *ty;
	StmtVar *decl;
};
class Layer
{
	std::unordered_map<std::string, TypeDecl> items;

public:
	inline bool add(const std::string &name, Type *val, StmtVar *decl)
	{
		if(exists(name)) return false;
		items[name] = {val, decl};
		return true;
	}
	inline bool exists(const std::string &name)
	{
		return items.find(name) != items.end();
	}
	inline Type *getTy(const std::string &name)
	{
		return items.find(name) == items.end() ? nullptr : items[name].ty;
	}
	inline StmtVar *getDecl(const std::string &name)
	{
		return items.find(name) == items.end() ? nullptr : items[name].decl;
	}
	inline std::unordered_map<std::string, TypeDecl> &getItems()
	{
		return items;
	}
};
class TypeManager
{
	std::unordered_map<std::string, TypeDecl> globals;
	std::vector<Layer> layers;
	std::unordered_map<uint64_t, std::unordered_map<std::string, FuncTy *>> typefuncs;
	std::vector<size_t> layerlock;
	std::vector<FuncTy *> funcstack;

public:
	TypeManager(Context &c);
	inline void pushLayer()
	{
		layers.emplace_back();
	}
	inline void popLayer()
	{
		layers.pop_back();
	}
	inline void pushFunc(FuncTy *fn)
	{
		funcstack.push_back(fn);
	}
	inline void popFunc()
	{
		funcstack.pop_back();
	}
	inline FuncTy *getTopFunc()
	{
		return funcstack.back();
	}
	inline bool hasFunc()
	{
		return !funcstack.empty();
	}
	inline size_t getCurrentLayerIndex()
	{
		return layers.size() - 1;
	}
	inline void lockScopeBelow(const size_t &idx)
	{
		layerlock.push_back(idx);
	}
	inline void unlockScope()
	{
		layerlock.pop_back();
	}
	bool addVar(const std::string &var, Type *val, StmtVar *decl, bool global = false);
	bool addTypeFn(Type *ty, const std::string &name, FuncTy *fn);
	bool addTypeFn(const uint64_t &id, const std::string &name, FuncTy *fn);
	bool exists(const std::string &var, bool top_only, bool include_globals);
	bool existsTypeFn(Type *ty, const std::string &name);
	Type *getTy(const std::string &var, bool top_only, bool include_globals);
	StmtVar *getDecl(const std::string &var, bool top_only, bool include_globals);
	FuncTy *getTypeFn(Type *ty, const std::string &name);
};
} // namespace sc

#endif // TYPEMGR_HPP