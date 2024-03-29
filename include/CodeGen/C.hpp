#pragma once

#include "Base.hpp"
#include "Writer.hpp"

namespace sc
{
struct CTy
{
	String base;
	String arr;

private:
	size_t recurse;
	size_t ptrs;
	size_t ptrsin;
	bool isstatic;
	bool isvolatile;
	bool isconst;
	bool isref;
	bool iscast;
	bool isdecl;
	bool isweak;

public:
	CTy();
	CTy(const String &base, const String &arr, size_t ptrs);

	CTy operator+(const CTy &other) const;
	CTy &operator+=(const CTy &other);

	String toStr(StringRef *varname);
	size_t size();
	void clear();

#define SetX(name, var) \
	inline void set##name(bool val) { var = val; }

#define IsX(name, var) \
	inline bool is##name() { return var; }

	SetX(Static, isstatic);
	SetX(Volatile, isvolatile);
	SetX(Const, isconst);
	SetX(Ref, isref);
	SetX(Cast, iscast);
	SetX(Decl, isdecl);
	SetX(Weak, isweak);

	IsX(Static, isstatic);
	IsX(Volatile, isvolatile);
	IsX(Const, isconst);
	IsX(Ref, isref);
	IsX(Cast, iscast);
	IsX(Decl, isdecl);
	IsX(Weak, isweak);

	inline void clearArray() { arr.clear(); }
	inline bool isArray() { return !arr.empty(); }

	inline void incRecurse() { ++recurse; }
	inline bool isTop() { return recurse == 0; }

	inline void incPtrs() { ++ptrs; }

	inline size_t getPtrs() { return ptrs; }

	inline void setPtrsIn(size_t _ptrsin) { ptrsin = _ptrsin; }
	inline size_t getPtrsIn() { return ptrsin; }
};

class CDriver : public CodeGenDriver
{
	Vector<StringRef> headerflags;
	Vector<StringRef> libflags;
	Vector<StringRef> preheadermacros;
	Vector<StringRef> headers;
	Vector<StringRef> macros;
	Vector<StringRef> typedefs;
	Vector<StringRef> structdecls;
	Vector<StringRef> funcptrs;
	Vector<StringRef> funcdecls;
	struct ConstantInfo
	{
		// var: names of const declarations
		// decl: C code equivalent
		StringRef var;
		StringRef decl;
	};
	// constants: key is the constant data
	Map<StringRef, ConstantInfo> constants;

	StringRef getConstantDataVar(const lex::Lexeme &val, Type *ty);
	StringRef getNewConstantVar();
	static bool acceptsSemicolon(Stmt *stmt);
	bool getCType(CTy &res, const ModuleLoc *loc, Type *ty);
	StringRef getCTypeForStringRef(Context &c, const ModuleLoc *loc);
	bool getCValue(String &res, Stmt *stmt, Value *value, Type *type, bool i8_to_char = true);
	bool addStructDef(const ModuleLoc *loc, StructTy *sty);
	bool writeCallArgs(const ModuleLoc *loc, const Vector<Stmt *> &args, Type *ty,
			   Writer &writer);
	bool applyCast(Stmt *stmt, Writer &writer, Writer &tmp);
	bool getFuncPointer(CTy &res, FuncTy *f, const ModuleLoc *loc);
	StringRef getArrCount(Type *t, size_t &ptrsin);
	StringRef getSystemCompiler();

	StringRef getMangledName(StringRef name, Type *ty);
	inline StringRef getMangledName(StringRef name, Stmt *stmt)
	{
		return getMangledName(name, stmt->getTy(true));
	}

public:
	CDriver(RAIIParser &parser);
	~CDriver() override;

	bool compile(StringRef outfile) override;

	bool visit(Stmt *stmt, Writer &writer, bool semicol);

	bool visit(StmtBlock *stmt, Writer &writer, bool semicol);
	bool visit(StmtType *stmt, Writer &writer, bool semicol);
	bool visit(StmtSimple *stmt, Writer &writer, bool semicol);
	bool visit(StmtFnCallInfo *stmt, Writer &writer, bool semicol);
	bool visit(StmtExpr *stmt, Writer &writer, bool semicol);
	bool visit(StmtVar *stmt, Writer &writer, bool semicol);
	bool visit(StmtFnSig *stmt, Writer &writer, bool semicol);
	bool visit(StmtFnDef *stmt, Writer &writer, bool semicol);
	bool visit(StmtHeader *stmt, Writer &writer, bool semicol);
	bool visit(StmtLib *stmt, Writer &writer, bool semicol);
	bool visit(StmtExtern *stmt, Writer &writer, bool semicol);
	bool visit(StmtEnum *stmt, Writer &writer, bool semicol);
	bool visit(StmtStruct *stmt, Writer &writer, bool semicol);
	bool visit(StmtVarDecl *stmt, Writer &writer, bool semicol);
	bool visit(StmtCond *stmt, Writer &writer, bool semicol);
	bool visit(StmtFor *stmt, Writer &writer, bool semicol);
	bool visit(StmtRet *stmt, Writer &writer, bool semicol);
	bool visit(StmtContinue *stmt, Writer &writer, bool semicol);
	bool visit(StmtBreak *stmt, Writer &writer, bool semicol);
	bool visit(StmtDefer *stmt, Writer &writer, bool semicol);
};
} // namespace sc
