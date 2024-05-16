#pragma once

#include <cassert>

#include "Lex.hpp"

namespace sc::AST
{

enum Stmts : uint8_t
{
	BLOCK,
	TYPE,
	SIMPLE,
	EXPR,
	FNCALLINFO,
	VAR,   // Var and (Type or Val) (<var> : <type> = <value>)
	FNSIG, // function signature
	FNDEF,
	HEADER, // header format for extern
	LIB,	// lib format for extern
	EXTERN,
	ENUMDEF,
	STRUCTDEF,
	VARDECL,
	COND,
	FOR,
	RET,
	CONTINUE,
	BREAK,
};

enum class StmtMask : uint16_t
{
	REF	 = 1 << 0,
	CONST	 = 1 << 1,
	COMPTIME = 1 << 2,
	STATIC	 = 1 << 3,
	VOLATILE = 1 << 4,
	IN	 = 1 << 5,
	GLOBAL	 = 1 << 6,
};

String genAnonymousName(StringRef suffix);

class Stmt
{
protected:
	const ModuleLoc *loc;
	Map<StringRef, StringRef> attrs;
	uint16_t derefcount; // number of dereferences to be done while generating code
	Stmts stype;
	uint16_t stmtmask; // for StmtMask

public:
	Stmt(Stmts stmt_type, const ModuleLoc *loc);
	virtual ~Stmt();

	virtual void disp(bool has_next)  = 0;
	virtual Stmt *clone(Context &ctx) = 0;

	const char *getStmtTypeCString() const;

#define isStmtX(X, ENUMVAL) \
	inline bool is##X() { return stype == ENUMVAL; }
	isStmtX(Block, BLOCK);
	isStmtX(Type, TYPE);
	isStmtX(Simple, SIMPLE);
	isStmtX(Expr, EXPR);
	isStmtX(FnCallInfo, FNCALLINFO);
	isStmtX(Var, VAR);
	isStmtX(FnSig, FNSIG);
	isStmtX(FnDef, FNDEF);
	isStmtX(Header, HEADER);
	isStmtX(Lib, LIB);
	isStmtX(Extern, EXTERN);
	isStmtX(EnumDef, ENUMDEF);
	isStmtX(StructDef, STRUCTDEF);
	isStmtX(VarDecl, VARDECL);
	isStmtX(Cond, COND);
	isStmtX(For, FOR);
	isStmtX(Return, RET);
	isStmtX(Continue, CONTINUE);
	isStmtX(Break, BREAK);

#define SetModifierX(Fn, Mod) \
	inline void set##Fn() { stmtmask |= (uint8_t)StmtMask::Mod; }
	SetModifierX(Ref, REF);
	SetModifierX(Const, CONST);
	SetModifierX(Comptime, COMPTIME);
	SetModifierX(Static, STATIC);
	SetModifierX(Volatile, VOLATILE);
	SetModifierX(In, IN);
	SetModifierX(Global, GLOBAL);
#undef SetModifierX

#define UnsetModifierX(Fn, Mod) \
	inline void unset##Fn() { stmtmask &= ~(uint8_t)StmtMask::Mod; }
	UnsetModifierX(Ref, REF);
	UnsetModifierX(Const, CONST);
	UnsetModifierX(Comptime, COMPTIME);
	UnsetModifierX(Static, STATIC);
	UnsetModifierX(Volatile, VOLATILE);
	UnsetModifierX(In, IN);
	UnsetModifierX(Global, GLOBAL);
#undef UnsetModifierX

#define IsModifierX(Fn, Mod) \
	inline bool is##Fn() const { return stmtmask & (uint8_t)StmtMask::Mod; }
	IsModifierX(Ref, REF);
	IsModifierX(Const, CONST);
	IsModifierX(Comptime, COMPTIME);
	IsModifierX(Static, STATIC);
	IsModifierX(Volatile, VOLATILE);
	IsModifierX(In, IN);
	IsModifierX(Global, GLOBAL);
#undef IsModifierX

	inline void setStmtMask(uint16_t mask) { stmtmask = mask; }
	inline uint16_t getStmtMask() { return stmtmask; }
	inline void appendStmtMask(uint16_t mask) { stmtmask |= mask; }
	inline const Stmts &getStmtType() const { return stype; }
	inline StringRef getStmtTypeString() const { return getStmtTypeCString(); }

	inline const ModuleLoc *getLoc() { return loc; }
	inline Module *getMod() const { return loc->getMod(); }
	inline void setDerefCount(uint16_t count) { derefcount = count; }
	inline uint16_t getDerefCount() { return derefcount; }

	inline bool hasAttribute(StringRef name) { return attrs.find(name) != attrs.end(); }
	inline void addAttribute(StringRef name, StringRef val = "") { attrs[String(name)] = val; }
	inline void setAttributes(const Map<StringRef, StringRef> &_attrs) { attrs = _attrs; }
	inline void setAttributes(Map<StringRef, StringRef> &&_attrs)
	{
		using namespace std;
		swap(attrs, _attrs);
	}
	StringRef getAttributeValue(StringRef name);
	String attributesToString(StringRef prefix = "", StringRef suffix = "");
};

template<typename T> T *as(AST::Stmt *data) { return static_cast<T *>(data); }

template<typename T> Stmt **asStmt(T **data) { return (Stmt **)(data); }

} // namespace sc::AST

namespace sc::err
{
template<typename... Args> void out(AST::Stmt *stmt, Args &&...args)
{
	out(stmt->getLoc(), std::forward<Args>(args)...);
}
template<typename... Args> void outw(AST::Stmt *stmt, Args &&...args)
{
	outw(stmt->getLoc(), std::forward<Args>(args)...);
}
} // namespace sc::err

namespace sc::AST
{
class StmtBlock : public Stmt
{
	Vector<Stmt *> stmts;
	bool is_top;

public:
	StmtBlock(const ModuleLoc *loc, const Vector<Stmt *> &stmts, bool is_top);
	~StmtBlock();
	static StmtBlock *create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &stmts,
				 bool is_top);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline Vector<Stmt *> &getStmts() { return stmts; }
	inline bool isTop() const { return is_top; }
};

class StmtType : public Stmt
{
	size_t ptr;    // number of ptrs
	bool variadic; // this is a variadic type

	Stmt *expr; // can be func, func call, name, name.x, ... (expr1)

public:
	StmtType(const ModuleLoc *loc, size_t ptr, bool variadic, Stmt *expr);
	~StmtType();
	static StmtType *create(Context &c, const ModuleLoc *loc, size_t ptr, bool variadic,
				Stmt *expr);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setVariadic() { variadic = true; }
	inline void unsetVariadic() { variadic = false; }

	inline size_t getPtrCount() const { return ptr; }

	inline bool isVariadic() const { return variadic; }

	inline Stmt *&getExpr() { return expr; }

	String getStringName();

	inline bool isFunc() const { return expr && expr->getStmtType() == FNSIG; }

	bool isMetaType() const;
};

class StmtVar;
class StmtSimple : public Stmt
{
	StmtVar *decl;
	lex::Lexeme val;
	Stmt *self; // for executing member functions

	bool disable_module_id_mangle;
	bool disable_codegen_mangle;

public:
	StmtSimple(const ModuleLoc *loc, const lex::Lexeme &val);
	~StmtSimple();
	static StmtSimple *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &val);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setDecl(StmtVar *d) { decl = d; }
	inline void setLexeme(const lex::Lexeme &newdata) { val = newdata; }
	inline void updateLexDataStr(StringRef newdata) { val.setDataStr(newdata); }
	inline void setSelf(Stmt *s) { self = s; }
	inline void disableModuleIDMangling() { disable_module_id_mangle = true; }
	inline void disableCodeGenMangling() { disable_codegen_mangle = true; }

	inline StmtVar *&getDecl() { return decl; }
	inline lex::Lexeme &getLexValue() { return val; }
	inline bool isLexTokType(lex::TokType ty) { return val.isType(ty); }
	inline Stmt *&getSelf() { return self; }
	inline bool isModuleIDManglingDisabled() const { return disable_module_id_mangle; }
	inline bool isCodeGenManglingDisabled() { return disable_codegen_mangle; }
};

class StmtFnCallInfo : public Stmt
{
	Vector<Stmt *> args;

public:
	StmtFnCallInfo(const ModuleLoc *loc, const Vector<Stmt *> &args);
	~StmtFnCallInfo();
	static StmtFnCallInfo *create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &args);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setArg(size_t idx, Stmt *a) { args[idx] = a; }
	inline Vector<Stmt *> &getArgs() { return args; }
	inline Stmt *getArg(size_t idx) { return args[idx]; }
};

class StmtExpr : public Stmt
{
	size_t commas;
	Stmt *lhs;
	lex::Lexeme oper;
	Stmt *rhs;
	StmtBlock *or_blk;
	lex::Lexeme or_blk_var;
	bool is_intrinsic_call;

public:
	StmtExpr(const ModuleLoc *loc, size_t commas, Stmt *lhs, const lex::Lexeme &oper, Stmt *rhs,
		 bool is_intrinsic_call);
	~StmtExpr();
	// or_blk and or_blk_var can be set separately - nullptr/INVALID by default
	static StmtExpr *create(Context &c, const ModuleLoc *loc, size_t commas, Stmt *lhs,
				const lex::Lexeme &oper, Stmt *rhs, bool is_intrinsic_call);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setCommas(size_t c) { commas = c; }
	inline void setOr(StmtBlock *blk, const lex::Lexeme &blk_var)
	{
		or_blk	   = blk;
		or_blk_var = blk_var;
	}

	inline size_t getCommas() const { return commas; }
	inline Stmt *&getLHS() { return lhs; }
	inline Stmt *&getRHS() { return rhs; }
	inline lex::Lexeme &getOper() { return oper; }
	inline StmtBlock *&getOrBlk() { return or_blk; }
	inline lex::Lexeme &getOrBlkVar() { return or_blk_var; }
	inline bool isIntrinsicCall() const { return is_intrinsic_call; }
};

class StmtVar : public Stmt
{
	lex::Lexeme name;
	StmtType *vtype;
	Stmt *vval; // either of expr, funcdef, enumdef, or structdef

public:
	StmtVar(const ModuleLoc *loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval);
	~StmtVar();
	// at least one of type or val must be present
	static StmtVar *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &name,
			       StmtType *vtype, Stmt *vval);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setVVal(Stmt *val) { vval = val; }

	inline lex::Lexeme &getName() { return name; }
	inline StmtType *&getVType() { return vtype; }
	inline Stmt *&getVVal() { return vval; }
};

class StmtFnSig : public Stmt
{
	// StmtVar contains type here, and optionally val for default arg values
	Vector<StmtVar *> args;
	StmtType *rettype;
	bool disable_template; // this function is in use, contains no template
	bool has_variadic;

public:
	StmtFnSig(const ModuleLoc *loc, Vector<StmtVar *> &args, StmtType *rettype,
		  bool has_variadic);
	~StmtFnSig();
	static StmtFnSig *create(Context &c, const ModuleLoc *loc, Vector<StmtVar *> &args,
				 StmtType *rettype, bool has_variadic);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void insertArg(StmtVar *arg) { args.push_back(arg); }
	inline void insertArg(size_t pos, StmtVar *arg) { args.insert(args.begin() + pos, arg); }
	inline void disableTemplates() { disable_template = true; }
	inline void setVariadic(bool va) { has_variadic = va; }

	inline StmtVar *&getArg(size_t idx) { return args[idx]; }
	inline Vector<StmtVar *> &getArgs() { return args; }
	inline StmtType *&getRetType() { return rettype; }
	inline bool hasTemplatesDisabled() { return disable_template; }
	inline bool hasVariadic() const { return has_variadic; }
};

class StmtFnDef : public Stmt
{
	String name;
	StmtFnSig *sig;
	StmtBlock *blk;
	StmtVar *parentvar;
	int64_t used; // if unused (false), will be deleted in SimplifyPass
	bool is_inline;

public:
	StmtFnDef(const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk, bool is_inline);
	~StmtFnDef();
	static StmtFnDef *create(Context &c, const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk,
				 bool is_inline);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setName(StringRef newName) { name = newName; }
	inline void setBlk(StmtBlock *_blk) { blk = _blk; }

	inline void setParentVar(StmtVar *pvar) { parentvar = pvar; }
	inline StringRef getName()
	{
		if(name.empty()) name = genAnonymousName("Func");
		return name;
	}
	inline StmtFnSig *&getSig() { return sig; }
	inline StmtBlock *&getBlk() { return blk; }
	inline StmtVar *&getParentVar() { return parentvar; }

	inline StmtVar *&getSigArg(size_t idx) { return sig->getArg(idx); }
	inline const Vector<StmtVar *> &getSigArgs() const { return sig->getArgs(); }
	inline StmtType *&getSigRetType() { return sig->getRetType(); }
	inline bool hasSigVariadic() const { return sig->hasVariadic(); }
	inline bool isUsed() { return used > 0; }
	inline int64_t getUsed() { return used; }
	inline bool isInline() { return is_inline; }
};

class StmtHeader : public Stmt
{
	// name is comma separated list of include files - along with bracket/quotes to be used,
	// flags is (optional) include cli parameters (space separated)
	lex::Lexeme names, flags;

public:
	StmtHeader(const ModuleLoc *loc, const lex::Lexeme &names, const lex::Lexeme &flags);
	static StmtHeader *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &names,
				  const lex::Lexeme &flags);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline const lex::Lexeme &getNames() const { return names; }
	inline const lex::Lexeme &getFlags() const { return flags; }
};

class StmtLib : public Stmt
{
	// flags is the space separated list of lib flags
	lex::Lexeme flags;

public:
	StmtLib(const ModuleLoc *loc, const lex::Lexeme &flags);
	static StmtLib *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &flags);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline const lex::Lexeme &getFlags() const { return flags; }
};

class StmtExtern : public Stmt
{
	lex::Lexeme fname; // name of the function/struct
	StmtHeader *headers;
	StmtLib *libs;
	Stmt *entity; // StmtFnSig or StmtStruct
	StmtVar *parentvar;

public:
	StmtExtern(const ModuleLoc *loc, const lex::Lexeme &fname, StmtHeader *headers,
		   StmtLib *libs, Stmt *entity);
	~StmtExtern();
	// headers and libs can be set separately - by default nullptr
	static StmtExtern *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &fname,
				  StmtHeader *headers, StmtLib *libs, Stmt *entity);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setParentVar(StmtVar *var) { parentvar = var; }

	inline const lex::Lexeme &getName() const { return fname; }

	inline StmtHeader *&getHeaders() { return headers; }

	inline StmtLib *&getLibs() { return libs; }

	inline Stmt *&getEntity() { return entity; }
	inline StmtVar *&getParentVar() { return parentvar; }
};

class StmtEnum : public Stmt
{
	String name;
	Vector<lex::Lexeme> items;
	StmtType *tagty; // optional

public:
	StmtEnum(const ModuleLoc *loc, const Vector<lex::Lexeme> &items, StmtType *tagty);
	~StmtEnum();
	static StmtEnum *create(Context &c, const ModuleLoc *loc, const Vector<lex::Lexeme> &items,
				StmtType *tagty);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setName(StringRef newName) { name = newName; }
	inline StringRef getName()
	{
		if(name.empty()) name = genAnonymousName("Enum");
		return name;
	}
	inline Vector<lex::Lexeme> &getItems() { return items; }
	inline StmtType *&getTagTy() { return tagty; }
};

// both declaration + definition, and struct + union
class StmtStruct : public Stmt
{
	String name;
	Vector<StmtVar *> fields;
	Vector<lex::Lexeme> templates;
	bool is_union;
	bool is_decl;
	bool is_externed; // required for setting up partial types correctly

public:
	StmtStruct(const ModuleLoc *loc, const Vector<StmtVar *> &fields,
		   const Vector<lex::Lexeme> &templates, bool is_union, bool is_decl);
	~StmtStruct();
	// StmtVar contains only type here, no val
	static StmtStruct *create(Context &c, const ModuleLoc *loc, const Vector<StmtVar *> &fields,
				  const Vector<lex::Lexeme> &templates, bool is_union,
				  bool is_decl);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setName(StringRef newName) { name = newName; }
	inline void setDecl(bool decl) { is_decl = decl; }
	inline void setExterned(bool externed) { is_externed = externed; }
	inline StringRef getName()
	{
		if(name.empty()) name = genAnonymousName("Struct");
		return name;
	}
	inline Vector<StmtVar *> &getFields() { return fields; }
	inline StmtVar *getField(size_t idx) { return fields[idx]; }
	inline const Vector<lex::Lexeme> &getTemplates() { return templates; }
	inline bool isUnion() { return is_union; }
	inline bool isDecl() { return is_decl; }
	inline bool isExterned() { return is_externed; }

	Vector<StringRef> getTemplateNames();
};

class StmtVarDecl : public Stmt
{
	Vector<StmtVar *> decls;

public:
	StmtVarDecl(const ModuleLoc *loc, const Vector<StmtVar *> &decls);
	~StmtVarDecl();
	// StmtVar can contain any combination of type, in, val(any), or all three
	static StmtVarDecl *create(Context &c, const ModuleLoc *loc,
				   const Vector<StmtVar *> &decls);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline Vector<StmtVar *> &getDecls() { return decls; }
};

class Conditional
{
	Stmt *cond; // can be nullptr (else)
	StmtBlock *blk;

public:
	Conditional(Stmt *cond, StmtBlock *blk);
	~Conditional();

	inline void reset()
	{
		cond = nullptr;
		blk  = nullptr;
	}

	inline Stmt *&getCond() { return cond; }
	inline StmtBlock *&getBlk() { return blk; }
	inline const Stmt *getCond() const { return cond; }
	inline const StmtBlock *getBlk() const { return blk; }
};

class StmtCond : public Stmt
{
	Vector<Conditional> conds;
	bool is_inline;

public:
	StmtCond(const ModuleLoc *loc, Vector<Conditional> &&conds, bool is_inline);
	StmtCond(const ModuleLoc *loc, const Vector<Conditional> &conds, bool is_inline);
	~StmtCond();
	static StmtCond *create(Context &c, const ModuleLoc *loc, Vector<Conditional> &&conds,
				bool is_inline);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline Vector<Conditional> &getConditionals() { return conds; }
	inline bool isInline() const { return is_inline; }
};

class StmtFor : public Stmt
{
	Stmt *init; // either of StmtVarDecl or StmtExpr
	Stmt *cond;
	Stmt *incr;
	StmtBlock *blk;
	bool is_inline;

public:
	StmtFor(const ModuleLoc *loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk,
		bool is_inline);
	~StmtFor();
	// init, cond, incr can be nullptr
	static StmtFor *create(Context &c, const ModuleLoc *loc, Stmt *init, Stmt *cond, Stmt *incr,
			       StmtBlock *blk, bool is_inline);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline Stmt *&getInit() { return init; }
	inline Stmt *&getCond() { return cond; }
	inline Stmt *&getIncr() { return incr; }
	inline StmtBlock *&getBlk() { return blk; }
	inline bool isInline() const { return is_inline; }
};

class StmtRet : public Stmt
{
	Stmt *val;
	StmtBlock *fnblk;

public:
	StmtRet(const ModuleLoc *loc, Stmt *val);
	~StmtRet();
	static StmtRet *create(Context &c, const ModuleLoc *loc, Stmt *val);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);

	inline void setFnBlk(StmtBlock *blk) { fnblk = blk; }

	inline Stmt *&getRetVal() { return val; }
	inline StmtBlock *&getFnBlk() { return fnblk; }
};

class StmtContinue : public Stmt
{
public:
	StmtContinue(const ModuleLoc *loc);
	static StmtContinue *create(Context &c, const ModuleLoc *loc);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
};

class StmtBreak : public Stmt
{
public:
	StmtBreak(const ModuleLoc *loc);
	static StmtBreak *create(Context &c, const ModuleLoc *loc);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
};
} // namespace sc::AST
