#include "Instructions.hpp"

namespace sc
{

Instruction::Instruction(ModuleLoc loc, Instructions instrty)
	: Value(loc, Values::INSTRUCTION), instrty(instrty)
{}
Instruction::Instruction(ModuleLoc loc, Instructions instrty, size_t argscapacity)
	: Value(loc, Values::INSTRUCTION), instrty(instrty)
{
	args.reserve(argscapacity);
}
Instruction::~Instruction() {}

LoadInstruction::LoadInstruction(ModuleLoc loc, SimpleValue *data)
	: Instruction(loc, Instructions::LOAD, 1)
{
	addArg(data);
}
LoadInstruction::~LoadInstruction() {}

String LoadInstruction::toString() const
{
	return utils::toString("load [", args[0]->toString(), "]");
}

} // namespace sc