#pragma once

#include <unordered_map>

#include "Parser/Stmts.hpp"

namespace sc
{
struct VarDecl
{
	Type *ty;
	Value *val;
	StmtVar *decl;
};
class Layer
{
	Map<StringRef, VarDecl> items;

public:
	inline bool add(StringRef name, Type *ty, Value *val, StmtVar *decl)
	{
		if(exists(name)) return false;
		items[name] = {ty, val, decl};
		return true;
	}
	inline bool exists(StringRef name) { return items.find(name) != items.end(); }
	inline VarDecl *getAll(StringRef name)
	{
		return items.find(name) == items.end() ? nullptr : &items[name];
	}
	inline Map<StringRef, VarDecl> &getItems() { return items; }
};
class Function
{
	FuncTy *fty;
	Vector<Layer> layers;

public:
	Function(FuncTy *ty);

	inline void setTy(FuncTy *ty) { fty = ty; }
	inline FuncTy *getFuncTy() { return fty; }
	inline void pushLayer() { layers.emplace_back(); }
	inline void popLayer() { layers.pop_back(); }
	inline size_t size() { return layers.size(); }
	inline bool add(StringRef name, Type *ty, Value *val, StmtVar *decl)
	{
		return layers.back().add(name, ty, val, decl);
	}
	bool exists(StringRef name, bool top_only);
	VarDecl *getAll(StringRef name, bool top_only);
};
class ValueManager
{
	Map<uint32_t, Map<StringRef, FuncVal *>> typefuncs;
	Vector<Function> funcstack;
	Layer globals;

public:
	ValueManager(Context &c);
	inline void pushLayer()
	{
		if(!funcstack.empty()) funcstack.back().pushLayer();
	}
	inline void popLayer()
	{
		if(!funcstack.empty()) funcstack.back().popLayer();
	}
	inline void pushFunc(FuncTy *fn)
	{
		funcstack.emplace_back(fn);
		funcstack.back().pushLayer();
	}
	inline void popFunc()
	{
		// who cares about popping layer when we're popping the func
		// funcstack.back().popLayer();
		funcstack.pop_back();
	}
	inline Function &getTopFunc() { return funcstack.back(); }
	inline bool hasFunc() { return !funcstack.empty(); }
	inline bool addTypeFn(Type *ty, StringRef name, FuncVal *fn)
	{
		return addTypeFn(ty->getID(), name, fn);
	}
	bool addVar(StringRef var, Type *ty, Value *val, StmtVar *decl, bool global = false);
	bool existsTypeFn(Type *ty, StringRef name);
	bool addTypeFn(uint32_t id, StringRef name, FuncVal *fn);
	bool exists(StringRef var, bool top_only);
	Type *getTy(StringRef var, bool top_only);
	Value *getVal(StringRef var, bool top_only);
	StmtVar *getDecl(StringRef var, bool top_only);
	VarDecl *getAll(StringRef name, bool top_only);
	FuncVal *getTyFn(Type *ty, StringRef name);
};
} // namespace sc
