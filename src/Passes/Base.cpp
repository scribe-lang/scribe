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

#include "Passes/Base.hpp"

namespace sc
{
Pass::Pass(const size_t &passid, Context &ctx) : passid(passid), ctx(ctx)
{
	ctx.addPass(passid, this);
}
Pass::~Pass() { ctx.remPass(passid); }

PassManager::PassManager(Context &ctx) : ctx(ctx) {}
PassManager::~PassManager()
{
	for(auto &p : passes) delete p;
}
bool PassManager::visit(Stmt *&ptree)
{
	for(auto &p : passes) {
		if(!p->visit(ptree, &ptree)) return false;
	}
	return true;
}
} // namespace sc