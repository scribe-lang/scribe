#pragma once

#include "Instructions.hpp"

namespace sc
{

class IRBuilder
{
	Allocator &allocator;
	Vector<Instruction *> &ir;

public:
	IRBuilder(Allocator &allocator, Vector<Instruction *> &ir);
	~IRBuilder();

	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<Instruction, T>::value, T>::type *
	addInst(Args &&...args)
	{
		T *res = T::create(allocator, std::forward<Args>(args)...);
		ir.push_back(res);
		return res;
	}

	inline Allocator &getAllocator() { return allocator; }
	inline Instruction *getLastInst() { return ir.back(); }

	static void dumpIR(OStream &os, const Vector<Instruction *> &ir);
};

} // namespace sc