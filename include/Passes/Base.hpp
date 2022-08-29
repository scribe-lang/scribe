/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef PASSES_BASE_HPP
#define PASSES_BASE_HPP

#include "Context.hpp"
#include "Parser/Stmts.hpp"

namespace sc
{
class Pass
{
protected:
	size_t passid;
	Context &ctx;

	// https://stackoverflow.com/questions/51332851/alternative-id-generators-for-types
	template<typename T> static inline std::uintptr_t passID()
	{
		// no need for static variable as function is inlined
		return reinterpret_cast<std::uintptr_t>(&passID<T>);
	}

public:
	Pass(const size_t &passid, Context &ctx);
	virtual ~Pass();

	template<typename T>
	static typename std::enable_if<std::is_base_of<Pass, T>::value, size_t>::type genPassID()
	{
		return (size_t)Pass::passID<T>();
	}

	template<typename T>
	typename std::enable_if<std::is_base_of<Pass, T>::value, bool>::type isPass() const
	{
		return passid == Pass::passID<T>();
	}

	virtual bool visit(Stmt *stmt, Stmt **source) = 0;

	virtual bool visit(StmtBlock *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtType *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtSimple *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtFnCallInfo *stmt, Stmt **source) = 0;
	virtual bool visit(StmtExpr *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtVar *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtFnSig *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtFnDef *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtHeader *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtLib *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtExtern *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtEnum *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtStruct *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtVarDecl *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtCond *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtFor *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtRet *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtContinue *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtBreak *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtDefer *stmt, Stmt **source)	= 0;

	inline const size_t &getPassID() { return passid; }
};

template<typename T> T *as(Pass *t) { return static_cast<T *>(t); }

class PassManager
{
	Context &ctx;
	Vector<Pass *> passes;

public:
	PassManager(Context &ctx);
	PassManager(const PassManager &pm) = delete;
	~PassManager();
	template<class T, typename... Args>
	typename std::enable_if<std::is_base_of<Pass, T>::value, void>::type add(Args... args)
	{
		passes.push_back(new T(ctx, args...));
	}
	bool visit(Stmt *&ptree);
};
} // namespace sc

#endif // PASSES_BASE_HPP