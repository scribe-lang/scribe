/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef PASSES_BASE_HPP
#define PASSES_BASE_HPP

#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "Context.hpp"
#include "Parser/Stmts.hpp"

namespace sc
{
class Pass
{
protected:
	ErrMgr &err;
	Context &ctx;

public:
	Pass(ErrMgr &err, Context &ctx);
	virtual ~Pass();

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
	virtual bool visit(StmtForIn *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtFor *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtWhile *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtRet *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtContinue *stmt, Stmt **source)	= 0;
	virtual bool visit(StmtBreak *stmt, Stmt **source)	= 0;
};

class PassManager
{
	ErrMgr &err;
	Context &ctx;
	std::vector<Pass *> passes;

public:
	PassManager(ErrMgr &err, Context &ctx);
	PassManager(const PassManager &pm) = delete;
	~PassManager();
	template<class T, typename... Args>
	typename std::enable_if<std::is_base_of<Pass, T>::value, void>::type add(Args... args)
	{
		passes.push_back(new T(err, ctx, args...));
	}
	bool visit(Stmt *&ptree);
};
} // namespace sc

#endif // PASSES_BASE_HPP