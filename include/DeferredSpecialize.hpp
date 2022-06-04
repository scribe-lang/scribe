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

#ifndef DEFERRED_SPECIALIZE_HPP
#define DEFERRED_SPECIALIZE_HPP

#include "Parser/Stmts.hpp"

namespace sc
{
struct DeferredSpecializeData
{
	uint32_t id; // original struct id (just the type id)
	Type **loc;  // update this type loc
	Vector<Type *> actualtypes;
};
struct DeferredSpecializeDataInternal
{
	uint32_t id; // original struct id (just the type id)
	Type **loc;  // update this type location
};
class DeferredSpecialize
{
	Vector<DeferredSpecializeData> list;
	Vector<DeferredSpecializeDataInternal> listinternal;

public:
	DeferredSpecialize();
	inline void pushData(uint32_t id, Type **loc, const Vector<Type *> &actualtypes)
	{
		list.push_back({id, loc, actualtypes});
	}
	inline void pushDataInternal(uint32_t id, Type **loc)
	{
		listinternal.push_back({id, loc});
	}
	bool specialize(uint32_t id, Context &c, const ModuleLoc *loc);
};
} // namespace sc

#endif // DEFERRED_SPECIALIZE_HPP