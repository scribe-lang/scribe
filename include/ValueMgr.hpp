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
struct ValDecl
{
	uint64_t valueid;
	StmtVar *decl;
};
class Layer
{
	std::unordered_map<std::string, ValDecl> items;

public:
	inline bool add(const std::string &name, const uint64_t &vid, StmtVar *decl)
	{
		if(exists(name)) return false;
		items[name] = {vid, decl};
		return true;
	}
	inline bool exists(const std::string &name)
	{
		return items.find(name) != items.end();
	}
	inline uint64_t getVal(const std::string &name)
	{
		return items.find(name) == items.end() ? 0 : items[name].valueid;
	}
	inline StmtVar *getDecl(const std::string &name)
	{
		return items.find(name) == items.end() ? nullptr : items[name].decl;
	}
	inline std::unordered_map<std::string, ValDecl> &getItems()
	{
		return items;
	}
};
class LayerStack
{
	std::vector<Layer> layers;

public:
	inline void pushLayer()
	{
		layers.emplace_back();
	}
	inline void popLayer()
	{
		layers.pop_back();
	}
	inline size_t size()
	{
		return layers.size();
	}
	inline bool add(const std::string &name, const uint64_t &vid, StmtVar *decl)
	{
		return layers.back().add(name, vid, decl);
	}
	bool exists(const std::string &name, const bool &top_only);
	uint64_t getVal(const std::string &name, const bool &top_only);
	StmtVar *getDecl(const std::string &name, const bool &top_only);
};
class Function : public LayerStack
{
	FuncTy *fty;

public:
	Function(FuncTy *ty);

	inline void setTy(FuncTy *ty)
	{
		fty = ty;
	}
	inline FuncTy *getTy()
	{
		return fty;
	}
};
class ValueManager
{
	std::unordered_map<uint64_t, std::unordered_map<std::string, uint64_t>> typefuncs;
	std::unordered_map<std::string, ValDecl> globals;
	LayerStack layers; // outside function
	std::vector<Function> funcstack;

public:
	ValueManager(Context &c);
	inline void pushLayer()
	{
		if(funcstack.empty()) {
			layers.pushLayer();
		} else {
			funcstack.back().pushLayer();
		}
	}
	inline void popLayer()
	{
		if(funcstack.empty()) {
			layers.popLayer();
		} else {
			funcstack.back().popLayer();
		}
	}
	inline void pushFunc(FuncTy *fn)
	{
		funcstack.emplace_back(fn);
	}
	inline void popFunc()
	{
		funcstack.pop_back();
	}
	inline Function &getTopFunc()
	{
		return funcstack.back();
	}
	inline bool hasFunc()
	{
		return !funcstack.empty();
	}
	bool addVar(const std::string &var, const uint64_t &vid, StmtVar *decl,
		    bool global = false);
	bool addTypeFn(Type *ty, const std::string &name, const uint64_t &fn);
	bool addTypeFn(const uint64_t &id, const std::string &name, const uint64_t &fn);
	bool exists(const std::string &var, bool top_only, bool include_globals);
	bool existsTypeFn(Type *ty, const std::string &name);
	uint64_t getVar(const std::string &var, bool top_only, bool include_globals);
	StmtVar *getDecl(const std::string &var, bool top_only, bool include_globals);
	uint64_t getTypeFn(Type *ty, const std::string &name);
};
} // namespace sc

#endif // TYPEMGR_HPP