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

#ifndef PARSER_STMTS_HPP
#define PARSER_STMTS_HPP

#include "Lex.hpp"
#include "Types.hpp"
#include "Values.hpp"

namespace sc
{
extern std::unordered_map<uint64_t, Value *> values;
uint64_t genValueID();
uint64_t createValueIDWith(Value *v);
Value *getValueWithID(const uint64_t &id);

enum Stmts
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
	FORIN,
	FOR,
	WHILE,
	RET,
	CONTINUE,
	BREAK,
	DEFER,
};

class Stmt
{
protected:
	Stmts stype;
	ModuleLoc loc;

	uint64_t valueid;
	Type *cast_to;
	size_t derefcount; // number of dereferences to be done while generating code

public:
	Stmt(const Stmts &stmt_type, const ModuleLoc &loc);
	virtual ~Stmt();

	virtual void disp(const bool &has_next) const = 0;
	virtual Stmt *clone(Context &ctx)	      = 0;
	virtual void clearValue()		      = 0;
	virtual bool requiresTemplateInit()	      = 0;
	virtual void _setFuncUsed(const bool &inc)    = 0;

	const char *getStmtTypeCString() const;
	std::string getTypeString() const;

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
	isStmtX(ForIn, FORIN);
	isStmtX(For, FOR);
	isStmtX(While, WHILE);
	isStmtX(Return, RET);
	isStmtX(Continue, CONTINUE);
	isStmtX(Break, BREAK);
	isStmtX(Defer, DEFER);

	inline const Stmts &getStmtType() const
	{
		return stype;
	}
	inline std::string getStmtTypeString() const
	{
		return getStmtTypeCString();
	}

	inline ModuleLoc &getLoc()
	{
		return loc;
	}
	inline Module *getMod() const
	{
		return loc.getMod();
	}

	inline void castTo(Type *t)
	{
		cast_to = t;
	}
	inline void setValueID(const uint64_t &vid)
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
	// changes the value at the valueid (valueid is unchanged)
	// changeValue() cannot exist because the inner (dot) members will be invalidated
	inline bool updateValue(Value *v)
	{
		assert(valueid && "valueid cannot be zero for updateValue()");
		if(!values[valueid]) {
			values[valueid] = v;
			return true;
		}
		return values[valueid]->updateValue(v);
	}
	inline void setValueTy(Type *t)
	{
		assert(valueid && "valueid cannot be zero for setValueTy()");
		values[valueid]->setType(t);
	}
	inline void setDerefCount(const size_t &count)
	{
		derefcount = count;
	}

	inline const uint64_t &getValueID()
	{
		return valueid;
	}
	Value *getValue();
	// if exact = true, cast will be skipped
	Type *getValueTy(const bool &exact = false);
	inline Type *getCast()
	{
		return cast_to;
	}
	inline const size_t &getDerefCount()
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
	std::vector<Stmt *> stmts;
	bool is_top;

public:
	StmtBlock(const ModuleLoc &loc, const std::vector<Stmt *> &stmts, const bool &is_top);
	~StmtBlock();
	static StmtBlock *create(Context &c, const ModuleLoc &loc, const std::vector<Stmt *> &stmts,
				 const bool &is_top);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline std::vector<Stmt *> &getStmts()
	{
		return stmts;
	}
	inline bool isTop() const
	{
		return is_top;
	}
};

class StmtType : public Stmt
{
	size_t ptr;  // number of ptrs
	size_t info; // all from TypeInfoMask

	Stmt *expr; // can be func, func call, name, name.x, ... (expr1)

public:
	StmtType(const ModuleLoc &loc, const size_t &ptr, const size_t &info, Stmt *expr);
	~StmtType();
	static StmtType *create(Context &c, const ModuleLoc &loc, const size_t &ptr,
				const size_t &info, Stmt *expr);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline void addTypeInfoMask(const size_t &mask)
	{
		info |= mask;
	}
	inline void remTypeInfoMask(const size_t &mask)
	{
		info &= ~mask;
	}
	inline const size_t &getPtrCount() const
	{
		return ptr;
	}
	inline const size_t &getInfoMask() const
	{
		return info;
	}

	bool hasModifier(const size_t &tim) const;

	inline Stmt *&getExpr()
	{
		return expr;
	}

	std::string getStringName();

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
	StmtSimple(const ModuleLoc &loc, const lex::Lexeme &val);
	~StmtSimple();
	static StmtSimple *create(Context &c, const ModuleLoc &loc, const lex::Lexeme &val);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline void setDecl(StmtVar *d)
	{
		decl = d;
	}
	inline void updateLexDataStr(const std::string &newdata)
	{
		val.setDataStr(newdata);
	}
	inline void setSelf(Stmt *s)
	{
		self = s;
	}
	inline void setAppliedModuleID(const bool &apply)
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
	std::vector<Stmt *> args;

public:
	StmtFnCallInfo(const ModuleLoc &loc, const std::vector<Stmt *> &args);
	~StmtFnCallInfo();
	static StmtFnCallInfo *create(Context &c, const ModuleLoc &loc,
				      const std::vector<Stmt *> &args);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline std::vector<Stmt *> &getArgs()
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
	StmtExpr(const ModuleLoc &loc, const size_t &commas, Stmt *lhs, const lex::Lexeme &oper,
		 Stmt *rhs, const bool &is_intrinsic_call);
	~StmtExpr();
	// or_blk and or_blk_var can be set separately - nullptr/INVALID by default
	static StmtExpr *create(Context &c, const ModuleLoc &loc, const size_t &commas, Stmt *lhs,
				const lex::Lexeme &oper, Stmt *rhs, const bool &is_intrinsic_call);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

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

class StmtVar : public Stmt
{
	lex::Lexeme name;
	StmtType *vtype;
	Stmt *vval; // either of expr, funcdef, enumdef, or structdef
	bool is_in;
	bool is_comptime;
	bool is_global;
	bool applied_module_id;
	bool applied_codegen_mangle;

public:
	StmtVar(const ModuleLoc &loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval,
		const bool &is_in, const bool &is_comptime, const bool &is_global);
	~StmtVar();
	// at least one of type or val must be present
	static StmtVar *create(Context &c, const ModuleLoc &loc, const lex::Lexeme &name,
			       StmtType *vtype, Stmt *vval, const bool &is_in,
			       const bool &is_comptime, const bool &is_global);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline void setVVal(Stmt *val)
	{
		vval = val;
	}
	inline void setCodeGenMangle(const bool &cgmangle)
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
	inline void setAppliedModuleID(const bool &apply)
	{
		applied_module_id = apply;
	}
	inline bool isIn()
	{
		return is_in;
	}
	inline bool isComptime() const
	{
		return is_comptime;
	}
	inline bool isGlobal() const
	{
		return is_global;
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
	std::vector<StmtVar *> args;
	StmtType *rettype;
	size_t scope;	       // for locking scopes during type assign
	bool disable_template; // this function is in use, contains no template
	bool has_variadic;

public:
	StmtFnSig(const ModuleLoc &loc, std::vector<StmtVar *> &args, StmtType *rettype,
		  const size_t &scope, const bool &has_variadic);
	~StmtFnSig();
	static StmtFnSig *create(Context &c, const ModuleLoc &loc, std::vector<StmtVar *> &args,
				 StmtType *rettype, const size_t &scope, const bool &has_variadic);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline void insertArg(const size_t &pos, StmtVar *arg)
	{
		args.insert(args.begin() + pos, arg);
	}
	inline void setScope(const size_t &s)
	{
		scope = s;
	}
	inline void disableTemplates()
	{
		disable_template = true;
	}
	inline void setVariadic(const bool &va)
	{
		has_variadic = va;
	}

	inline StmtVar *&getArg(const size_t &idx)
	{
		return args[idx];
	}
	inline std::vector<StmtVar *> &getArgs()
	{
		return args;
	}
	inline StmtType *&getRetType()
	{
		return rettype;
	}
	inline const size_t &getScope() const
	{
		return scope;
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
	StmtFnDef(const ModuleLoc &loc, StmtFnSig *sig, StmtBlock *blk);
	~StmtFnDef();
	static StmtFnDef *create(Context &c, const ModuleLoc &loc, StmtFnSig *sig, StmtBlock *blk);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline void setParentVar(StmtVar *pvar)
	{
		parentvar = pvar;
	}
	inline void incUsed()
	{
		_setFuncUsed(true);
	}
	inline void decUsed()
	{
		_setFuncUsed(false);
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
	inline const std::vector<StmtVar *> &getSigArgs() const
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
	StmtHeader(const ModuleLoc &loc, const lex::Lexeme &names, const lex::Lexeme &flags);
	static StmtHeader *create(Context &c, const ModuleLoc &loc, const lex::Lexeme &names,
				  const lex::Lexeme &flags);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

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
	StmtLib(const ModuleLoc &loc, const lex::Lexeme &flags);
	static StmtLib *create(Context &c, const ModuleLoc &loc, const lex::Lexeme &flags);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

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
	StmtExtern(const ModuleLoc &loc, const lex::Lexeme &fname, StmtHeader *headers,
		   StmtLib *libs, Stmt *entity);
	~StmtExtern();
	// headers and libs can be set separately - by default nullptr
	static StmtExtern *create(Context &c, const ModuleLoc &loc, const lex::Lexeme &fname,
				  StmtHeader *headers, StmtLib *libs, Stmt *entity);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

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
	std::vector<lex::Lexeme> items;

public:
	StmtEnum(const ModuleLoc &loc, const std::vector<lex::Lexeme> &items);
	~StmtEnum();
	static StmtEnum *create(Context &c, const ModuleLoc &loc,
				const std::vector<lex::Lexeme> &items);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline const std::vector<lex::Lexeme> &getItems() const
	{
		return items;
	}
};

// both declaration and definition
class StmtStruct : public Stmt
{
	std::vector<StmtVar *> fields;
	std::vector<lex::Lexeme> templates;
	bool is_externed; // required for setting up partial types correctly

public:
	StmtStruct(const ModuleLoc &loc, const std::vector<StmtVar *> &fields,
		   const std::vector<lex::Lexeme> &templates);
	~StmtStruct();
	// StmtVar contains only type here, no val
	static StmtStruct *create(Context &c, const ModuleLoc &loc,
				  const std::vector<StmtVar *> &fields,
				  const std::vector<lex::Lexeme> &templates);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline void setExterned(const bool &externed)
	{
		is_externed = externed;
	}
	inline std::vector<StmtVar *> &getFields()
	{
		return fields;
	}
	inline const std::vector<lex::Lexeme> &getTemplates()
	{
		return templates;
	}
	inline bool isExterned()
	{
		return is_externed;
	}

	std::vector<std::string> getTemplateNames();
};

class StmtVarDecl : public Stmt
{
	std::vector<StmtVar *> decls;

public:
	StmtVarDecl(const ModuleLoc &loc, const std::vector<StmtVar *> &decls);
	~StmtVarDecl();
	// StmtVar can contain any combination of type, in, val(any), or all three
	static StmtVarDecl *create(Context &c, const ModuleLoc &loc,
				   const std::vector<StmtVar *> &decls);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline std::vector<StmtVar *> &getDecls()
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
	std::vector<Conditional> conds;
	bool is_inline;

public:
	StmtCond(const ModuleLoc &loc, const std::vector<Conditional> &conds,
		 const bool &is_inline);
	~StmtCond();
	static StmtCond *create(Context &c, const ModuleLoc &loc,
				const std::vector<Conditional> &conds, const bool &is_inline);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline std::vector<Conditional> &getConditionals()
	{
		return conds;
	}
	inline bool isInline() const
	{
		return is_inline;
	}
};

class StmtForIn : public Stmt
{
	lex::Lexeme iter;
	Stmt *in; // L01
	StmtBlock *blk;

public:
	StmtForIn(const ModuleLoc &loc, const lex::Lexeme &iter, Stmt *in, StmtBlock *blk);
	~StmtForIn();
	static StmtForIn *create(Context &c, const ModuleLoc &loc, const lex::Lexeme &iter,
				 Stmt *in, StmtBlock *blk);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline const lex::Lexeme &getIter() const
	{
		return iter;
	}
	inline Stmt *&getIn()
	{
		return in;
	}
	inline StmtBlock *&getBlk()
	{
		return blk;
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
	StmtFor(const ModuleLoc &loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk,
		const bool &is_inline);
	~StmtFor();
	// init, cond, incr can be nullptr
	static StmtFor *create(Context &c, const ModuleLoc &loc, Stmt *init, Stmt *cond, Stmt *incr,
			       StmtBlock *blk, const bool &is_inline);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

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

class StmtWhile : public Stmt
{
	Stmt *cond;
	StmtBlock *blk;

public:
	StmtWhile(const ModuleLoc &loc, Stmt *cond, StmtBlock *blk);
	~StmtWhile();
	static StmtWhile *create(Context &c, const ModuleLoc &loc, Stmt *cond, StmtBlock *blk);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline Stmt *&getCond()
	{
		return cond;
	}
	inline StmtBlock *&getBlk()
	{
		return blk;
	}
};

class StmtRet : public Stmt
{
	Stmt *val;
	StmtBlock *fnblk;

public:
	StmtRet(const ModuleLoc &loc, Stmt *val);
	~StmtRet();
	static StmtRet *create(Context &c, const ModuleLoc &loc, Stmt *val);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

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
	StmtContinue(const ModuleLoc &loc);
	static StmtContinue *create(Context &c, const ModuleLoc &loc);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);
};

class StmtBreak : public Stmt
{
public:
	StmtBreak(const ModuleLoc &loc);
	static StmtBreak *create(Context &c, const ModuleLoc &loc);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);
};

class StmtDefer : public Stmt
{
	Stmt *val;

public:
	StmtDefer(const ModuleLoc &loc, Stmt *val);
	~StmtDefer();
	static StmtDefer *create(Context &c, const ModuleLoc &loc, Stmt *val);

	void disp(const bool &has_next) const;
	Stmt *clone(Context &ctx);
	void clearValue();
	bool requiresTemplateInit();
	void _setFuncUsed(const bool &inc);

	inline Stmt *&getVal()
	{
		return val;
	}
};
} // namespace sc

#endif // PARSER_STMTS_HPP