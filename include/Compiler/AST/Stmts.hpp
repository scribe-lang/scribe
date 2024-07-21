#pragma once

#include "Allocator.hpp"
#include "Lex.hpp"

namespace sc::ast
{

enum Stmts : uint8_t
{
	BLOCK,
	TYPE,
	SIMPLE,
	EXPR,
	CALLARGS,
	VAR,	   // Var and (Type or Val) (<var> : <type> = <value>)
	SIGNATURE, // function / struct signature
	FNDEF,
	VARDECL,
	COND,
	FOR,
	ONEWORD, // return, break, continue, defer, goto
};

class Stmt : public IAllocated
{
protected:
	ModuleLoc loc;
	StringMap<String> attrs;
	uint16_t derefcount; // number of dereferences to be done while generating code
	Stmts stype;

public:
	Stmt(Stmts stmt_type, ModuleLoc loc);
	virtual ~Stmt();

	virtual void disp(OStream &os, bool has_next) = 0;

	const char *getStmtTypeCString() const;

#define isStmtX(X, ENUMVAL) \
	inline bool is##X() { return stype == ENUMVAL; }
	isStmtX(Block, BLOCK);
	isStmtX(Type, TYPE);
	isStmtX(Simple, SIMPLE);
	isStmtX(Expr, EXPR);
	isStmtX(CallArgs, CALLARGS);
	isStmtX(Var, VAR);
	isStmtX(Signature, SIGNATURE);
	isStmtX(FnDef, FNDEF);
	isStmtX(VarDecl, VARDECL);
	isStmtX(Cond, COND);
	isStmtX(For, FOR);
	isStmtX(OneWord, ONEWORD);

	inline const Stmts &getStmtType() const { return stype; }
	inline StringRef getStmtTypeString() const { return getStmtTypeCString(); }

	inline const ModuleLoc &getLoc() { return loc; }
	inline void setDerefCount(uint16_t count) { derefcount = count; }
	inline uint16_t getDerefCount() { return derefcount; }

	inline bool hasAttribute(StringRef name) { return attrs.find(name) != attrs.end(); }
	inline void addAttribute(StringRef name, StringRef val = "") { attrs[String(name)] = val; }
	inline void setAttributes(const StringMap<String> &_attrs) { attrs = _attrs; }
	inline void setAttributes(StringMap<String> &&_attrs)
	{
		using namespace std;
		swap(attrs, _attrs);
	}
	inline const StringMap<String> &getAttributes() const { return attrs; }
	StringRef getAttributeValue(StringRef name);
	String attributesToString(StringRef prefix = "", StringRef suffix = "");
};

template<typename T> T *as(ast::Stmt *data) { return static_cast<T *>(data); }

template<typename T> Stmt **asStmt(T **data) { return (Stmt **)(data); }

} // namespace sc::ast

namespace sc::err
{
template<typename... Args> void out(ast::Stmt *stmt, Args &&...args)
{
	out(stmt->getLoc(), std::forward<Args>(args)...);
}
template<typename... Args> void outw(ast::Stmt *stmt, Args &&...args)
{
	outw(stmt->getLoc(), std::forward<Args>(args)...);
}
} // namespace sc::err

namespace sc::ast
{

class StmtBlock : public Stmt
{
	Vector<Stmt *> stmts;
	bool is_top;

public:
	StmtBlock(ModuleLoc loc, const Vector<Stmt *> &stmts, bool is_top);
	~StmtBlock();
	static StmtBlock *create(Allocator &allocator, ModuleLoc loc, const Vector<Stmt *> &stmts,
				 bool is_top);

	void disp(OStream &os, bool has_next);

	inline Vector<Stmt *> &getStmts() { return stmts; }
	inline bool isTop() const { return is_top; }
};

class StmtType : public Stmt
{
	Stmt *expr; // can be func, func call, name, name.x, ... (expr1)

public:
	StmtType(ModuleLoc loc, Stmt *expr);
	~StmtType();
	static StmtType *create(Allocator &allocator, ModuleLoc loc, Stmt *expr);

	void disp(OStream &os, bool has_next);

	inline Stmt *&getExpr() { return expr; }

	bool isMetaType() const;
};

class StmtVar;
class StmtSimple : public Stmt
{
	lex::Lexeme val;

public:
	StmtSimple(ModuleLoc loc, const lex::Lexeme &val);
	~StmtSimple();
	static StmtSimple *create(Allocator &allocator, ModuleLoc loc, const lex::Lexeme &val);

	void disp(OStream &os, bool has_next);

	inline void setLexeme(const lex::Lexeme &newdata) { val = newdata; }
	inline void updateLexDataStr(StringRef newdata) { val.setDataStr(newdata); }

	inline lex::Lexeme &getLexeme() { return val; }
	inline bool isLexTokType(lex::TokType ty) { return val.isType(ty); }
};

class StmtCallArgs : public Stmt
{
	Vector<Stmt *> args;

public:
	StmtCallArgs(ModuleLoc loc, const Vector<Stmt *> &args);
	~StmtCallArgs();
	static StmtCallArgs *create(Allocator &allocator, ModuleLoc loc,
				    const Vector<Stmt *> &args);

	void disp(OStream &os, bool has_next);

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
	StmtExpr(ModuleLoc loc, size_t commas, Stmt *lhs, const lex::Lexeme &oper, Stmt *rhs,
		 bool is_intrinsic_call);
	~StmtExpr();
	// or_blk and or_blk_var can be set separately - nullptr/INVALID by default
	static StmtExpr *create(Allocator &allocator, ModuleLoc loc, size_t commas, Stmt *lhs,
				const lex::Lexeme &oper, Stmt *rhs, bool is_intrinsic_call);

	void disp(OStream &os, bool has_next);

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
	StmtVar(ModuleLoc loc, const lex::Lexeme &name, StmtType *vtype, Stmt *vval);
	~StmtVar();
	// at least one of type or val must be present
	static StmtVar *create(Allocator &allocator, ModuleLoc loc, const lex::Lexeme &name,
			       StmtType *vtype, Stmt *vval);

	void disp(OStream &os, bool has_next);

	inline void setVVal(Stmt *val) { vval = val; }

	inline lex::Lexeme &getName() { return name; }
	inline StmtType *&getVType() { return vtype; }
	inline Stmt *&getVVal() { return vval; }
};

enum class SignatureType : uint8_t
{
	STRUCT,
	UNION,
	FUNC,
};

class StmtSignature : public Stmt
{
	// StmtVar contains type here, and optionally val for default arg values
	Vector<StmtVar *> args;
	StmtType *rettype; // mandatory for sigty == FUNC
	SignatureType sigty;

public:
	StmtSignature(ModuleLoc loc, Vector<StmtVar *> &args, StmtType *rettype,
		      SignatureType sigty);
	~StmtSignature();
	static StmtSignature *create(Allocator &allocator, ModuleLoc loc, Vector<StmtVar *> &args,
				     StmtType *rettype, SignatureType sigty);

	void disp(OStream &os, bool has_next);

	inline StmtVar *&getArg(size_t idx) { return args[idx]; }
	inline Vector<StmtVar *> &getArgs() { return args; }
	inline StmtType *&getRetType() { return rettype; }
	inline bool isStruct() { return sigty == SignatureType::STRUCT; }
	inline bool isUnion() { return sigty == SignatureType::UNION; }
	inline bool isFunc() { return sigty == SignatureType::FUNC; }
};

class StmtFnDef : public Stmt
{
	StmtSignature *sig;
	StmtBlock *blk;

public:
	StmtFnDef(ModuleLoc loc, StmtSignature *sig, StmtBlock *blk);
	~StmtFnDef();
	static StmtFnDef *create(Allocator &allocator, ModuleLoc loc, StmtSignature *sig,
				 StmtBlock *blk);

	void disp(OStream &os, bool has_next);

	inline StmtSignature *&getSig() { return sig; }
	inline StmtBlock *&getBlk() { return blk; }

	inline StmtVar *&getSigArg(size_t idx) { return sig->getArg(idx); }
	inline const Vector<StmtVar *> &getSigArgs() const { return sig->getArgs(); }
	inline StmtType *&getSigRetType() { return sig->getRetType(); }
	inline bool sigHasAttribute(StringRef attr) const { return sig->hasAttribute(attr); }
};

class StmtVarDecl : public Stmt
{
	Vector<StmtVar *> decls;

public:
	StmtVarDecl(ModuleLoc loc, const Vector<StmtVar *> &decls);
	~StmtVarDecl();
	// StmtVar can contain any combination of type, in, val(any), or all three
	static StmtVarDecl *create(Allocator &allocator, ModuleLoc loc,
				   const Vector<StmtVar *> &decls);

	void disp(OStream &os, bool has_next);

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
	StmtCond(ModuleLoc loc, Vector<Conditional> &&conds, bool is_inline);
	StmtCond(ModuleLoc loc, const Vector<Conditional> &conds, bool is_inline);
	~StmtCond();
	static StmtCond *create(Allocator &allocator, ModuleLoc loc, Vector<Conditional> &&conds,
				bool is_inline);

	void disp(OStream &os, bool has_next);

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
	StmtFor(ModuleLoc loc, Stmt *init, Stmt *cond, Stmt *incr, StmtBlock *blk, bool is_inline);
	~StmtFor();
	// init, cond, incr can be nullptr
	static StmtFor *create(Allocator &allocator, ModuleLoc loc, Stmt *init, Stmt *cond,
			       Stmt *incr, StmtBlock *blk, bool is_inline);

	void disp(OStream &os, bool has_next);

	inline Stmt *&getInit() { return init; }
	inline Stmt *&getCond() { return cond; }
	inline Stmt *&getIncr() { return incr; }
	inline StmtBlock *&getBlk() { return blk; }
	inline bool isInline() const { return is_inline; }
};

enum class OneWordType : uint8_t
{
	RETURN,
	CONTINUE,
	BREAK,
	DEFER,
	GOTO,
};
class StmtOneWord : public Stmt
{
	Stmt *arg; // optional value for return; required for defer/goto
	OneWordType wordty;

public:
	StmtOneWord(ModuleLoc loc, Stmt *arg, OneWordType wordty);
	~StmtOneWord();
	static StmtOneWord *create(Allocator &allocator, ModuleLoc loc, Stmt *arg,
				   OneWordType wordty);

	void disp(OStream &os, bool has_next);

#define isWordTy(X, ENUMVAL) \
	inline bool is##X() { return wordty == OneWordType::ENUMVAL; }
	isWordTy(Return, RETURN);
	isWordTy(Continue, CONTINUE);
	isWordTy(Break, BREAK);
	isWordTy(Defer, DEFER);
	isWordTy(Goto, GOTO);

	inline Stmt *&getArg() { return arg; }
};

} // namespace sc::ast
