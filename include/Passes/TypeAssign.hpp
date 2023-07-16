#pragma once

#include "DeferredSpecialize.hpp"
#include "DeferStack.hpp"
#include "Passes/Base.hpp"
#include "Passes/ValueAssign.hpp"
#include "ValueMgr.hpp"

namespace sc
{
class TypeAssignPass : public Pass
{
	ValueManager vmgr;
	ValueAssignPass vpass;
	DeferStack deferstack;
	// for cross-referencing structs
	DeferredSpecialize deferredspecialize;
	// each enum's (tag) type - used by @enumTy()
	Map<StringRef, Type *> enumtagtys;
	// vars created during type assign - enums, specialized funcs
	Vector<StmtVar *> additionalvars;
	// variadic length of current function
	Vector<size_t> valen;
	Vector<bool> is_fn_va;
	bool disabled_varname_mangling;

	StringRef getMangledName(Stmt *stmt, StringRef name, NamespaceVal *ns = nullptr) const;
	void applyPrimitiveTypeCoercion(Type *to, Stmt *from);
	void applyPrimitiveTypeCoercion(Stmt *lhs, Stmt *rhs, const lex::Lexeme &oper);
	bool chooseSuperiorPrimitiveType(Type *l, Type *r);
	bool initTemplateFunc(Stmt *caller, FuncTy *&cf, Vector<Stmt *> &args);

	void pushFunc(FuncVal *fn, bool is_va, size_t va_len);
	void updateLastFunc(FuncVal *fn, bool is_va, size_t va_len);
	void popFunc();

public:
	TypeAssignPass(Context &ctx);
	~TypeAssignPass() override;

	bool visit(Stmt *stmt, Stmt **source) override;

	bool visit(StmtBlock *stmt, Stmt **source) override;
	bool visit(StmtType *stmt, Stmt **source) override;
	bool visit(StmtSimple *stmt, Stmt **source) override;
	bool visit(StmtFnCallInfo *stmt, Stmt **source) override;
	bool visit(StmtExpr *stmt, Stmt **source) override;
	bool visit(StmtVar *stmt, Stmt **source) override;
	bool visit(StmtFnSig *stmt, Stmt **source) override;
	bool visit(StmtFnDef *stmt, Stmt **source) override;
	bool visit(StmtHeader *stmt, Stmt **source) override;
	bool visit(StmtLib *stmt, Stmt **source) override;
	bool visit(StmtExtern *stmt, Stmt **source) override;
	bool visit(StmtEnum *stmt, Stmt **source) override;
	bool visit(StmtStruct *stmt, Stmt **source) override;
	bool visit(StmtVarDecl *stmt, Stmt **source) override;
	bool visit(StmtCond *stmt, Stmt **source) override;
	bool visit(StmtFor *stmt, Stmt **source) override;
	bool visit(StmtRet *stmt, Stmt **source) override;
	bool visit(StmtContinue *stmt, Stmt **source) override;
	bool visit(StmtBreak *stmt, Stmt **source) override;
	bool visit(StmtDefer *stmt, Stmt **source) override;

	inline void addEnumTagTy(StringRef name, Type *ty) { enumtagtys[name] = ty; }
	inline Type *getEnumTagTy(StringRef name) const
	{
		auto found = enumtagtys.find(name);
		if(found == enumtagtys.end()) return nullptr;
		return found->second;
	}
	inline size_t getFnVALen() const { return valen.size() > 0 ? valen.back() : 0; }
	inline bool isFnVALen() const { return is_fn_va.size() > 0 ? is_fn_va.back() : false; }
};
} // namespace sc
