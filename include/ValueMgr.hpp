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
	Map<StringRef, ValDecl> items;

public:
	inline bool add(StringRef name, uint64_t vid, StmtVar *decl)
	{
		if(exists(name)) return false;
		items[name] = {vid, decl};
		return true;
	}
	inline bool exists(StringRef name)
	{
		return items.find(name) != items.end();
	}
	inline uint64_t getVal(StringRef name)
	{
		return items.find(name) == items.end() ? 0 : items[name].valueid;
	}
	inline StmtVar *getDecl(StringRef name)
	{
		return items.find(name) == items.end() ? nullptr : items[name].decl;
	}
	inline Map<StringRef, ValDecl> &getItems()
	{
		return items;
	}
};
class LayerStack
{
	Vector<Layer> layers;

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
	inline bool add(StringRef name, uint64_t vid, StmtVar *decl)
	{
		return layers.back().add(name, vid, decl);
	}
	bool exists(StringRef name, bool top_only);
	uint64_t getVal(StringRef name, bool top_only);
	StmtVar *getDecl(StringRef name, bool top_only);
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
	Map<uint64_t, Map<StringRef, uint64_t>> typefuncs;
	Map<StringRef, ValDecl> globals;
	LayerStack layers; // outside function
	Vector<Function> funcstack;

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
	inline bool isTop()
	{
		return funcstack.empty() && layers.size() == 1;
	}
	bool addVar(StringRef var, uint64_t vid, StmtVar *decl, bool global = false);
	bool addTypeFn(Type *ty, StringRef name, uint64_t fn);
	bool addTypeFn(uint64_t id, StringRef name, uint64_t fn);
	bool exists(StringRef var, bool top_only, bool include_globals);
	bool existsTypeFn(Type *ty, StringRef name);
	uint64_t getVar(StringRef var, bool top_only, bool include_globals);
	StmtVar *getDecl(StringRef var, bool top_only, bool include_globals);
	uint64_t getTypeFn(Type *ty, StringRef name);
};
} // namespace sc

#endif // TYPEMGR_HPP