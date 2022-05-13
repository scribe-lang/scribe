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

#ifndef PARSER_STMTS_HPP
#define PARSER_STMTS_HPP

#include <cassert>

#include "Lex.hpp"
#include "Types.hpp"
#include "Values.hpp"

namespace sc
{
extern Map<uint64_t, Value *> values;
uint64_t genValueID();
uint64_t createValueIDWith(Value *v);
Value *getValueWithID(uint64_t id);

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
	DEFER,
};

enum class StmtMask : uint8_t
{
	REF	 = 1 << 0,
	CONST	 = 1 << 1,
	COMPTIME = 1 << 2,
};

class Stmt
{
protected:
	const ModuleLoc *loc;
	uint64_t valueid;
	Type *cast_to;
	uint16_t derefcount; // number of dereferences to be done while generating code
	Stmts stype;
	uint8_t stmtmask; // for StmtMask
	uint8_t castmask;

public:
	Stmt(const Stmts &stmt_type, const ModuleLoc *loc);
	virtual ~Stmt();

	virtual void disp(bool has_next)		       = 0;
	virtual Stmt *clone(Context &ctx)		       = 0;
	virtual void clearValue()			       = 0;
	virtual bool requiresTemplateInit()		       = 0;
	virtual void _setFuncUsed(bool inc, Set<Stmt *> &done) = 0;

	const char *getStmtTypeCString() const;
	String getTypeString();

#define isStmtX(X, ENUMVAL)              \
	inline bool is##X()              \
	{                                \
		return stype == ENUMVAL; \
	}
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
	isStmtX(Defer, DEFER);

#define SetModifierX(Fn, Mod)                       \
	inline void set##Fn()                       \
	{                                           \
		stmtmask |= (uint8_t)StmtMask::Mod; \
	}
	SetModifierX(Ref, REF);
	SetModifierX(Const, CONST);
	SetModifierX(Comptime, COMPTIME);
#undef SetModifierX

#define UnsetModifierX(Fn, Mod)                      \
	inline void unset##Fn()                      \
	{                                            \
		stmtmask &= ~(uint8_t)StmtMask::Mod; \
	}
	UnsetModifierX(Ref, REF);
	UnsetModifierX(Const, CONST);
	UnsetModifierX(Comptime, COMPTIME);
#undef UnsetModifierX

#define IsModifierX(Fn, Mod)                              \
	inline bool is##Fn() const                        \
	{                                                 \
		return stmtmask & (uint8_t)StmtMask::Mod; \
	}
	IsModifierX(Ref, REF);
	IsModifierX(Const, CONST);
	IsModifierX(Comptime, COMPTIME);
#undef IsModifierX

#define SetCastModifierX(Fn, Mod)                   \
	inline void setCast##Fn()                   \
	{                                           \
		castmask |= (uint8_t)StmtMask::Mod; \
	}
	SetCastModifierX(Ref, REF);
	SetCastModifierX(Const, CONST);
	SetCastModifierX(Comptime, COMPTIME);
#undef SetCastModifierX

#define UnsetCastModifierX(Fn, Mod)                  \
	inline void unsetCast##Fn()                  \
	{                                            \
		castmask &= ~(uint8_t)StmtMask::Mod; \
	}
	UnsetCastModifierX(Ref, REF);
	UnsetCastModifierX(Const, CONST);
	UnsetCastModifierX(Comptime, COMPTIME);
#undef UnsetCastModifierX

#define IsCastModifierX(Fn, Mod)                          \
	inline bool isCast##Fn() const                    \
	{                                                 \
		return castmask & (uint8_t)StmtMask::Mod; \
	}
	IsCastModifierX(Ref, REF);
	IsCastModifierX(Const, CONST);
	IsCastModifierX(Comptime, COMPTIME);
#undef IsCastModifierX

	inline void setStmtMask(uint8_t mask)
	{
		stmtmask = mask;
	}
	inline void appendStmtMask(uint8_t mask)
	{
		stmtmask |= mask;
	}
	inline uint8_t getStmtMask()
	{
		return stmtmask;
	}

	inline void setCastStmtMask(uint8_t mask)
	{
		castmask = mask;
	}
	inline void appendCastStmtMask(uint8_t mask)
	{
		castmask |= mask;
	}
	inline uint8_t getCastStmtMask()
	{
		return castmask;
	}

	inline const Stmts &getStmtType() const
	{
		return stype;
	}
	inline StringRef getStmtTypeString() const
	{
		return getStmtTypeCString();
	}

	inline const ModuleLoc *getLoc()
	{
		return loc;
	}
	inline Module *getMod() const
	{
		return loc->getMod();
	}

	inline void castTo(Type *t, uint8_t maskfrom)
	{
		cast_to = t;
		appendCastStmtMask(maskfrom);
	}
	inline void setValueID(uint64_t vid)
	{
		valueid = vid;
	}
	inline void setValueID(Stmt *stmt)
	{
		valueid = stmt->getValueID();
	}
	// creates a new valueid and sets the value for it
	inline void createAndSetValue(Value *v)
	{
		valueid		= genValueID();
		values[valueid] = v;
	}
	inline void setValueTy(Type *t)
	{
		assert(valueid && "valueid cannot be zero for setValueTy()");
		values[valueid]->setType(t);
	}
	inline void setDerefCount(uint16_t count)
	{
		derefcount = count;
	}

	inline uint64_t getValueID()
	{
		return valueid;
	}
	// changes the value at the valueid (valueid is unchanged)
	inline void changeValue(Value *v)
	{
		values[valueid] = v;
	}
	// updates data in this->value using v
	bool updateValue(Context &c, Value *v);
	// if exact = true, cast and deref will be skipped
	Value *getValue(bool exact = false);
	// if exact = true, cast and deref will be skipped
	Type *getValueTy(bool exact = false);
	inline Type *getCast()
	{
		return cast_to;
	}
	inline uint16_t getDerefCount()
	{
		return derefcount;
	}
};

template<typename T> T *as(Stmt *data)
{
	return static_cast<T *>(data);
}

template<typename T> Stmt **asStmt(T **data)
{
	return (Stmt **)(data);
}

class StmtBlock : public Stmt
{
	Vector<Stmt *> stmts;
	bool is_top;
	bool disable_layering;

public:
	StmtBlock(const ModuleLoc *loc, const Vector<Stmt *> &stmts, bool is_top,
		  bool disable_layering);
	~StmtBlock();
	static StmtBlock *create(Context &c, const ModuleLoc *loc, const Vector<Stmt *> &stmts,
				 bool is_top, bool disable_layering);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void disableLayering()
	{
		disable_layering = true;
	}

	inline Vector<Stmt *> &getStmts()
	{
		return stmts;
	}
	inline bool isTop() const
	{
		return is_top;
	}
	inline bool isLayeringDisabled() const
	{
		return disable_layering;
	}
};

class StmtType : public Stmt
{
	size_t ptr;    // number of ptrs
	bool variadic; // this is a variadic type

	Stmt *expr; // can be func, func call, name, name.x, ... (expr1)

public:
	StmtType(const ModuleLoc *loc, const size_t &ptr, bool variadic, Stmt *expr);
	~StmtType();
	static StmtType *create(Context &c, const ModuleLoc *loc, const size_t &ptr, bool variadic,
				Stmt *expr);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void setVariadic()
	{
		variadic = true;
	}
	inline void unsetVariadic()
	{
		variadic = false;
	}

	inline const size_t &getPtrCount() const
	{
		return ptr;
	}

	inline bool isVariadic() const
	{
		return variadic;
	}

	inline Stmt *&getExpr()
	{
		return expr;
	}

	String getStringName();

	inline bool isFunc() const
	{
		return expr && expr->getStmtType() == FNSIG;
	}

	bool isMetaType() const;
};

class StmtVar;
class StmtSimple : public Stmt
{
	StmtVar *decl;
	lex::Lexeme val;
	Stmt *self; // for executing member functions

	bool applied_module_id;

public:
	StmtSimple(const ModuleLoc *loc, const lex::Lexeme &val);
	~StmtSimple();
	static StmtSimple *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &val);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void setDecl(StmtVar *d)
	{
		decl = d;
	}
	inline void updateLexDataStr(StringRef newdata)
	{
		val.setDataStr(newdata);
	}
	inline void setSelf(Stmt *s)
	{
		self = s;
	}
	inline void setAppliedModuleID(bool apply)
	{
		applied_module_id = apply;
	}
	inline StmtVar *&getDecl()
	{
		return decl;
	}
	inline lex::Lexeme &getLexValue()
	{
		return val;
	}
	inline Stmt *&getSelf()
	{
		return self;
	}
	inline bool isAppliedModuleID() const
	{
		return applied_module_id;
	}
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
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void setArg(const size_t &idx, Stmt *a)
	{
		args[idx] = a;
	}
	inline Vector<Stmt *> &getArgs()
	{
		return args;
	}
	inline Stmt *getArg(const size_t &idx)
	{
		return args[idx];
	}
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
	FuncTy *calledfn;

public:
	StmtExpr(const ModuleLoc *loc, const size_t &commas, Stmt *lhs, const lex::Lexeme &oper,
		 Stmt *rhs, bool is_intrinsic_call);
	~StmtExpr();
	// or_blk and or_blk_var can be set separately - nullptr/INVALID by default
	static StmtExpr *create(Context &c, const ModuleLoc *loc, const size_t &commas, Stmt *lhs,
				const lex::Lexeme &oper, Stmt *rhs, bool is_intrinsic_call);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void setCommas(const size_t &c)
	{
		commas = c;
	}
	inline void setOr(StmtBlock *blk, const lex::Lexeme &blk_var)
	{
		or_blk	   = blk;
		or_blk_var = blk_var;
	}
	inline void setCalledFnTy(FuncTy *calledty)
	{
		calledfn = calledty;
	}

	inline const size_t &getCommas() const
	{
		return commas;
	}
	inline Stmt *&getLHS()
	{
		return lhs;
	}
	inline Stmt *&getRHS()
	{
		return rhs;
	}
	inline lex::Lexeme &getOper()
	{
		return oper;
	}
	inline StmtBlock *&getOrBlk()
	{
		return or_blk;
	}
	inline lex::Lexeme &getOrBlkVar()
	{
		return or_blk_var;
	}
	inline bool isIntrinsicCall() const
	{
		return is_intrinsic_call;
	}
	inline FuncTy *getCalledFn()
	{
		return calledfn;
	}
};

enum class VarMask : uint8_t
{
	STATIC	 = 1 << 0,
	VOLATILE = 1 << 1,
	IN	 = 1 << 2,
	GLOBAL	 = 1 << 3,
};

class StmtVar : public Stmt
{
	lex::Lexeme name;
	StmtType *vtype;
	Stmt *vval;	 // either of expr, funcdef, enumdef, or structdef
	uint8_t varmask; // from VarMask
	bool applied_module_id;
	bool applied_codegen_mangle;

public:
	StmtVar(const ModuleLoc *loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval,
		uint8_t varmask);
	~StmtVar();
	// at least one of type or val must be present
	static StmtVar *create(Context &c, const ModuleLoc *loc, const lex::Lexeme &name,
			       StmtType *vtype, Stmt *vval, uint8_t varmask);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

#define SetModifierX(Fn, Mod)                     \
	inline void set##Fn()                     \
	{                                         \
		varmask |= (uint8_t)VarMask::Mod; \
	}
	SetModifierX(Static, STATIC);
	SetModifierX(Volatile, VOLATILE);
	SetModifierX(In, IN);
	SetModifierX(Global, GLOBAL);
#undef SetModifierX

#define UnsetModifierX(Fn, Mod)                    \
	inline void unset##Fn()                    \
	{                                          \
		varmask &= ~(uint8_t)VarMask::Mod; \
	}
	UnsetModifierX(Static, STATIC);
	UnsetModifierX(Volatile, VOLATILE);
	UnsetModifierX(In, IN);
	UnsetModifierX(Global, GLOBAL);
#undef UnsetModifierX

#define IsModifierX(Fn, Mod)                            \
	inline bool is##Fn() const                      \
	{                                               \
		return varmask & (uint8_t)VarMask::Mod; \
	}
	IsModifierX(Static, STATIC);
	IsModifierX(Volatile, VOLATILE);
	IsModifierX(In, IN);
	IsModifierX(Global, GLOBAL);
#undef IsModifierX

	inline void setVVal(Stmt *val)
	{
		vval = val;
	}
	inline void setCodeGenMangle(bool cgmangle)
	{
		applied_codegen_mangle = cgmangle;
	}

	inline lex::Lexeme &getName()
	{
		return name;
	}
	inline StmtType *&getVType()
	{
		return vtype;
	}
	inline Stmt *&getVVal()
	{
		return vval;
	}
	inline void setAppliedModuleID(bool apply)
	{
		applied_module_id = apply;
	}
	inline bool isAppliedModuleID() const
	{
		return applied_module_id;
	}
	inline bool isCodeGenMangled()
	{
		return applied_codegen_mangle;
	}
};

class StmtFnSig : public Stmt
{
	// StmtVar contains only type here, no val
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
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void insertArg(StmtVar *arg)
	{
		args.push_back(arg);
	}
	inline void insertArg(const size_t &pos, StmtVar *arg)
	{
		args.insert(args.begin() + pos, arg);
	}
	inline void disableTemplates()
	{
		disable_template = true;
	}
	inline void setVariadic(bool va)
	{
		has_variadic = va;
	}

	inline StmtVar *&getArg(const size_t &idx)
	{
		return args[idx];
	}
	inline Vector<StmtVar *> &getArgs()
	{
		return args;
	}
	inline StmtType *&getRetType()
	{
		return rettype;
	}
	inline bool hasTemplatesDisabled()
	{
		return disable_template;
	}
	inline bool hasVariadic() const
	{
		return has_variadic;
	}
};

class StmtFnDef : public Stmt
{
	StmtFnSig *sig;
	StmtBlock *blk;
	StmtVar *parentvar;
	int64_t used; // if unused (false), will be deleted in SimplifyPass

public:
	StmtFnDef(const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk);
	~StmtFnDef();
	static StmtFnDef *create(Context &c, const ModuleLoc *loc, StmtFnSig *sig, StmtBlock *blk);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void setBlk(StmtBlock *_blk)
	{
		blk = _blk;
	}

	inline void setParentVar(StmtVar *pvar)
	{
		parentvar = pvar;
	}
	inline void incUsed()
	{
		Set<Stmt *> done;
		_setFuncUsed(true, done);
	}
	inline void decUsed()
	{
		Set<Stmt *> done;
		_setFuncUsed(false, done);
	}
	inline StmtFnSig *&getSig()
	{
		return sig;
	}
	inline StmtBlock *&getBlk()
	{
		return blk;
	}
	inline StmtVar *&getParentVar()
	{
		return parentvar;
	}

	inline StmtVar *&getSigArg(const size_t &idx)
	{
		return sig->getArg(idx);
	}
	inline const Vector<StmtVar *> &getSigArgs() const
	{
		return sig->getArgs();
	}
	inline StmtType *&getSigRetType()
	{
		return sig->getRetType();
	}
	inline bool hasSigVariadic() const
	{
		return sig->hasVariadic();
	}
	inline bool isUsed()
	{
		return used > 0;
	}
	inline int64_t getUsed()
	{
		return used;
	}
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
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline const lex::Lexeme &getNames() const
	{
		return names;
	}
	inline const lex::Lexeme &getFlags() const
	{
		return flags;
	}
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
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline const lex::Lexeme &getFlags() const
	{
		return flags;
	}
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
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void setParentVar(StmtVar *var)
	{
		parentvar = var;
	}

	inline const lex::Lexeme &getName() const
	{
		return fname;
	}

	inline StmtHeader *&getHeaders()
	{
		return headers;
	}

	inline StmtLib *&getLibs()
	{
		return libs;
	}

	inline Stmt *&getEntity()
	{
		return entity;
	}
	inline StmtVar *&getParentVar()
	{
		return parentvar;
	}
};

class StmtEnum : public Stmt
{
	Vector<lex::Lexeme> items;

public:
	StmtEnum(const ModuleLoc *loc, const Vector<lex::Lexeme> &items);
	~StmtEnum();
	static StmtEnum *create(Context &c, const ModuleLoc *loc, const Vector<lex::Lexeme> &items);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline Vector<lex::Lexeme> &getItems()
	{
		return items;
	}
};

// both declaration and definition
class StmtStruct : public Stmt
{
	Vector<StmtVar *> fields;
	Vector<lex::Lexeme> templates;
	bool is_externed; // required for setting up partial types correctly

public:
	StmtStruct(const ModuleLoc *loc, const Vector<StmtVar *> &fields,
		   const Vector<lex::Lexeme> &templates);
	~StmtStruct();
	// StmtVar contains only type here, no val
	static StmtStruct *create(Context &c, const ModuleLoc *loc, const Vector<StmtVar *> &fields,
				  const Vector<lex::Lexeme> &templates);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void setExterned(bool externed)
	{
		is_externed = externed;
	}
	inline Vector<StmtVar *> &getFields()
	{
		return fields;
	}
	inline StmtVar *getField(size_t idx)
	{
		return fields[idx];
	}
	inline const Vector<lex::Lexeme> &getTemplates()
	{
		return templates;
	}
	inline bool isExterned()
	{
		return is_externed;
	}

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
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline Vector<StmtVar *> &getDecls()
	{
		return decls;
	}
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

	inline Stmt *&getCond()
	{
		return cond;
	}
	inline StmtBlock *&getBlk()
	{
		return blk;
	}
	inline const Stmt *getCond() const
	{
		return cond;
	}
	inline const StmtBlock *getBlk() const
	{
		return blk;
	}
};

class StmtCond : public Stmt
{
	Vector<Conditional> conds;
	bool is_inline;

public:
	StmtCond(const ModuleLoc *loc, const Vector<Conditional> &conds, bool is_inline);
	~StmtCond();
	static StmtCond *create(Context &c, const ModuleLoc *loc, const Vector<Conditional> &conds,
				bool is_inline);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline Vector<Conditional> &getConditionals()
	{
		return conds;
	}
	inline bool isInline() const
	{
		return is_inline;
	}
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
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline Stmt *&getInit()
	{
		return init;
	}
	inline Stmt *&getCond()
	{
		return cond;
	}
	inline Stmt *&getIncr()
	{
		return incr;
	}
	inline StmtBlock *&getBlk()
	{
		return blk;
	}
	inline bool isInline() const
	{
		return is_inline;
	}
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
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline void setFnBlk(StmtBlock *blk)
	{
		fnblk = blk;
	}

	inline Stmt *&getVal()
	{
		return val;
	}
	inline StmtBlock *&getFnBlk()
	{
		return fnblk;
	}
};

class StmtContinue : public Stmt
{
public:
	StmtContinue(const ModuleLoc *loc);
	static StmtContinue *create(Context &c, const ModuleLoc *loc);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);
};

class StmtBreak : public Stmt
{
public:
	StmtBreak(const ModuleLoc *loc);
	static StmtBreak *create(Context &c, const ModuleLoc *loc);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);
};

class StmtDefer : public Stmt
{
	Stmt *val;

public:
	StmtDefer(const ModuleLoc *loc, Stmt *val);
	~StmtDefer();
	static StmtDefer *create(Context &c, const ModuleLoc *loc, Stmt *val);

	void disp(bool has_next);
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(bool inc, Set<Stmt *> &done);

	inline Stmt *&getVal()
	{
		return val;
	}
};
} // namespace sc

#endif // PARSER_STMTS_HPP