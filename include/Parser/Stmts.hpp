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
	Stmts stype;
	ModuleLoc loc;

	Stmt *parent;

	Type *type;
	Type *cast_from;
	Value *value;

	size_t specialized_id;

	// generally used by intrinsics, causes values to never be erased or modified
	// can be overridden by calling setPermaValue() again
	bool is_value_perma;

	bool is_comptime;

public:
	Stmt(const Stmts &stmt_type, const ModuleLoc &loc);
	virtual ~Stmt();
	Stmt *getParentWithType(const Stmts &ty, Stmt **childofparent = nullptr);

	virtual void disp(const bool &has_next) const = 0;

	inline const Stmts &getStmtType() const
	{
		return stype;
	}
	const char *getStmtTypeCString() const;
	inline std::string getStmtTypeString() const
	{
		return getStmtTypeCString();
	}

	std::string getTypeString() const;

	inline ModuleLoc &getLoc()
	{
		return loc;
	}
	inline Module *getMod() const
	{
		return loc.getMod();
	}
	inline Stmt *&getParent()
	{
		return parent;
	}

	inline void setParent(Stmt *par)
	{
		parent = par;
	}
	inline void setType(Type *t)
	{
		type = t;
	}
	inline void castTo(Type *t)
	{
		cast_from = type;
		type	  = t;
	}
	inline void setVal(Value *v)
	{
		if(is_value_perma) return;
		value = v;
	}
	inline void setSpecializedID(const size_t &id)
	{
		specialized_id = id;
	}
	inline void setPermaVal(Value *v)
	{
		value	       = v;
		is_value_perma = true;
	}
	inline void setComptime(const bool &comptime)
	{
		is_comptime = comptime;
	}

	inline Type *&getType()
	{
		return type;
	}
	inline Value *&getValue()
	{
		return value;
	}
	inline const size_t &getSpecializedID() const
	{
		return specialized_id;
	}
	inline bool isComptime() const
	{
		return is_comptime;
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

class StmtType : public Stmt
{
	size_t ptr;  // number of ptrs
	size_t info; // all from TypeInfoMask

	std::vector<Stmt *> array_counts; // all array counts (if any)
	Stmt *expr;			  // can be func, func call, name, name.x, ... (expr1)

public:
	StmtType(const ModuleLoc &loc, const size_t &ptr, const size_t &info,
		 const std::vector<Stmt *> &array_counts, Stmt *expr);
	~StmtType();

	void disp(const bool &has_next) const;

	inline void addTypeInfoMask(const size_t &mask)
	{
		info |= mask;
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

	inline std::vector<Stmt *> &getArrayCounts()
	{
		return array_counts;
	}
	inline Stmt *&getExpr()
	{
		return expr;
	}

	std::string getStringName();

	inline bool isFunc() const
	{
		return expr && expr->getStmtType() == FNSIG;
	}
};

class StmtBlock : public Stmt
{
	std::vector<Stmt *> stmts;
	bool is_top;

public:
	StmtBlock(const ModuleLoc &loc, const std::vector<Stmt *> &stmts, const bool &is_top);
	~StmtBlock();

	void disp(const bool &has_next) const;

	inline std::vector<Stmt *> &getStmts()
	{
		return stmts;
	}
	inline bool isTop() const
	{
		return is_top;
	}
};

class StmtSimple : public Stmt
{
	lex::Lexeme val;

	bool applied_module_id;

public:
	StmtSimple(const ModuleLoc &loc, const lex::Lexeme &val);
	~StmtSimple();

	void disp(const bool &has_next) const;

	inline void updateLexDataStr(const std::string &newdata)
	{
		val.setDataStr(newdata);
	}
	inline void setAppliedModuleID(const bool &apply)
	{
		applied_module_id = apply;
	}
	inline lex::Lexeme &getLexValue()
	{
		return val;
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

	void disp(const bool &has_next) const;

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

public:
	// or_blk and or_blk_var can be set separately - nullptr/INVALID by default
	StmtExpr(const ModuleLoc &loc, const size_t &commas, Stmt *lhs, const lex::Lexeme &oper,
		 Stmt *rhs, const bool &is_intrinsic_call);
	~StmtExpr();

	void disp(const bool &has_next) const;

	inline void setCommas(const size_t &c)
	{
		commas = c;
	}
	inline void setOr(StmtBlock *blk, const lex::Lexeme &blk_var)
	{
		or_blk	   = blk;
		or_blk_var = blk_var;
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
};

class StmtVar : public Stmt
{
	lex::Lexeme name;
	StmtType *in;
	StmtType *vtype;
	Stmt *vval; // either of expr, funcdef, enumdef, or structdef
	bool is_comptime;
	bool is_global;

public:
	// at least one of type or val must be present
	StmtVar(const ModuleLoc &loc, const lex::Lexeme &name, StmtType *in, StmtType *vtype,
		Stmt *vval, const bool &is_comptime, const bool &is_global);
	~StmtVar();

	void disp(const bool &has_next) const;

	inline lex::Lexeme &getName()
	{
		return name;
	}
	inline StmtType *&getIn()
	{
		return in;
	}
	inline StmtType *&getVType()
	{
		return vtype;
	}
	inline Stmt *&getVVal()
	{
		return vval;
	}
	inline bool isComptime() const
	{
		return is_comptime;
	}
	inline bool isGlobal() const
	{
		return is_global;
	}
};

class StmtFnSig : public Stmt
{
	// StmtVar contains only type here, no val
	std::vector<StmtVar *> args;
	StmtType *rettype;
	bool has_variadic;
	bool is_member;

public:
	StmtFnSig(const ModuleLoc &loc, std::vector<StmtVar *> &args, StmtType *rettype,
		  const bool &has_variadic, const bool &is_member);
	~StmtFnSig();

	void disp(const bool &has_next) const;

	inline void insertArg(const size_t &pos, StmtVar *arg)
	{
		args.insert(args.begin() + pos, arg);
	}
	inline void setMember(const bool &member)
	{
		is_member = member;
	}

	inline const std::vector<StmtVar *> &getArgs() const
	{
		return args;
	}
	inline StmtType *&getRetType()
	{
		return rettype;
	}
	inline bool hasVariadic() const
	{
		return has_variadic;
	}
	inline bool isMember() const
	{
		return is_member;
	}
};

class StmtFnDef : public Stmt
{
	StmtFnSig *sig;
	StmtBlock *blk;

public:
	StmtFnDef(const ModuleLoc &loc, StmtFnSig *sig, StmtBlock *blk);
	~StmtFnDef();

	void disp(const bool &has_next) const;

	inline StmtFnSig *&getSig()
	{
		return sig;
	}
	inline StmtBlock *&getBlk()
	{
		return blk;
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
	inline bool isSigMember() const
	{
		return sig->isMember();
	}
};

class StmtHeader : public Stmt
{
	// name is comma separated list of include files - along with bracket/quotes to be used,
	// flags is (optional) include cli parameters (space separated)
	lex::Lexeme names, flags;

public:
	StmtHeader(const ModuleLoc &loc, const lex::Lexeme &names, const lex::Lexeme &flags);

	void disp(const bool &has_next) const;

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

	void disp(const bool &has_next) const;

	inline const lex::Lexeme &getFlags() const
	{
		return flags;
	}
};

class StmtExtern : public Stmt
{
	lex::Lexeme fname; // name of the function
	StmtHeader *headers;
	StmtLib *libs;
	StmtFnSig *sig;

public:
	// headers and libs can be set separately - by default nullptr
	StmtExtern(const ModuleLoc &loc, const lex::Lexeme &fname, StmtHeader *headers,
		   StmtLib *libs, StmtFnSig *sig);
	~StmtExtern();

	void disp(const bool &has_next) const;

	inline const lex::Lexeme &getFnName() const
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

	inline StmtFnSig *&getSig()
	{
		return sig;
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
	inline bool isSigMember() const
	{
		return sig->isMember();
	}
};

class StmtEnum : public Stmt
{
	std::vector<lex::Lexeme> items;

public:
	// StmtVar contains only val(expr) here, no type
	StmtEnum(const ModuleLoc &loc, const std::vector<lex::Lexeme> &items);
	~StmtEnum();

	void disp(const bool &has_next) const;

	inline const std::vector<lex::Lexeme> &getItems() const
	{
		return items;
	}
};

// both declaration and definition
class StmtStruct : public Stmt
{
	std::vector<StmtVar *> fields;

public:
	// StmtVar contains only type here, no val
	StmtStruct(const ModuleLoc &loc, const std::vector<StmtVar *> &fields);
	~StmtStruct();

	void disp(const bool &has_next) const;

	inline const std::vector<StmtVar *> &getFields() const
	{
		return fields;
	}
};

class StmtVarDecl : public Stmt
{
	std::vector<StmtVar *> decls;

public:
	// StmtVar can contain any combination of type, in, val(any), or all three
	StmtVarDecl(const ModuleLoc &loc, const std::vector<StmtVar *> &decls);
	~StmtVarDecl();

	void disp(const bool &has_next) const;

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

	void disp(const bool &has_next) const;

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

	void disp(const bool &has_next) const;

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
	// init, cond, incr can be nullptr
	StmtFor(const ModuleLoc &loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk,
		const bool &is_inline);
	~StmtFor();

	void disp(const bool &has_next) const;

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

	void disp(const bool &has_next) const;

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

public:
	StmtRet(const ModuleLoc &loc, Stmt *val);
	~StmtRet();

	void disp(const bool &has_next) const;

	inline Stmt *&getVal()
	{
		return val;
	}
};
class StmtContinue : public Stmt
{
public:
	StmtContinue(const ModuleLoc &loc);

	void disp(const bool &has_next) const;
};
class StmtBreak : public Stmt
{
public:
	StmtBreak(const ModuleLoc &loc);

	void disp(const bool &has_next) const;
};
class StmtDefer : public Stmt
{
	Stmt *val;

public:
	StmtDefer(const ModuleLoc &loc, Stmt *val);
	~StmtDefer();

	void disp(const bool &has_next) const;

	inline Stmt *&getVal()
	{
		return val;
	}
};
} // namespace sc

#endif // PARSER_STMTS_HPP