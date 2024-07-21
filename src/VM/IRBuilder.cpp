#include "IRBuilder.hpp"

namespace sc
{

IRBuilder::IRBuilder(Allocator &allocator, Vector<Instruction *> &ir) : allocator(allocator), ir(ir)
{}
IRBuilder::~IRBuilder() {}

void IRBuilder::dumpIR(OStream &os, const Vector<Instruction *> &ir)
{
	for(auto &i : ir) os << i->toString() << "\n";
}

} // namespace sc