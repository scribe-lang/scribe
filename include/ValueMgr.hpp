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
class LayerStack
{
	Vector<Layer> layers;

public:
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
class Function : public LayerStack
{
	FuncTy *fty;

public:
	Function(FuncTy *ty);

	inline void setTy(FuncTy *ty) { fty = ty; }
	inline FuncTy *getFuncTy() { return fty; }
};
class ValueManager
{
	Map<uint32_t, Map<StringRef, FuncVal *>> typefuncs;
	Map<StringRef, VarDecl> globals;
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
	inline void pushFunc(FuncTy *fn) { funcstack.emplace_back(fn); }
	inline void popFunc() { funcstack.pop_back(); }
	inline Function &getTopFunc() { return funcstack.back(); }
	inline bool hasFunc() { return !funcstack.empty(); }
	inline bool isTop() { return funcstack.empty() && layers.size() == 1; }
	bool addVar(StringRef var, Type *ty, Value *val, StmtVar *decl, bool global = false);
	bool addTypeFn(Type *ty, StringRef name, FuncVal *fn);
	bool addTypeFn(uint32_t id, StringRef name, FuncVal *fn);
	bool exists(StringRef var, bool top_only, bool include_globals);
	bool existsTypeFn(Type *ty, StringRef name);
	Type *getTy(StringRef var, bool top_only, bool include_globals);
	Value *getVal(StringRef var, bool top_only, bool include_globals);
	StmtVar *getDecl(StringRef var, bool top_only, bool include_globals);
	VarDecl *getAll(StringRef name, bool top_only, bool include_globals);
	FuncVal *getTyFn(Type *ty, StringRef name);
};
} // namespace sc
