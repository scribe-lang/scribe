#pragma once

#include "Types.hpp"

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
	inline void pushDataInternal(uint32_t id, Type **loc) { listinternal.push_back({id, loc}); }
	bool specialize(uint32_t id, Context &c, const ModuleLoc *loc);
};
} // namespace sc
