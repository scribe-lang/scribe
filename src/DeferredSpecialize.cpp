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

#include "DeferredSpecialize.hpp"

#include "Error.hpp"

namespace sc
{
DeferredSpecialize::DeferredSpecialize() {}
bool DeferredSpecialize::specialize(uint32_t id, Context &c, const ModuleLoc *loc)
{
	for(auto it = list.begin(); it != list.end();) {
		if(it->id == id) {
			StructTy *ty = as<StructTy>((*it->loc));
			ty	     = ty->applyTemplates(c, loc, it->actualtypes);
			if(!ty) {
				err::out(*loc, {"Failed to delay specialize"
						" type: ",
						(*it->loc)->toStr()});
				return false;
			}
			for(auto it = listinternal.begin(); it != listinternal.end();) {
				if(it->id == id) {
					*it->loc = ty;
					it	 = listinternal.erase(it);
					continue;
				}
				++it;
			}
			*it->loc = ty;
			ty->getDecl()->setDecl(false);
			it = list.erase(it);
			continue;
		}
		++it;
	}
	return true;
}
} // namespace sc