#include "Instructions.hpp"

namespace sc
{

Instruction::Instruction(ModuleLoc loc, Instructions instrty)
	: Value(loc, Values::INSTRUCTION), instrty(instrty)
{}
Instruction::~Instruction() {}

LoadInstruction::LoadInstruction(ModuleLoc loc, SimpleValue *data)
	: Instruction(loc, Instructions::LOAD), data(data)
{}
LoadInstruction::~LoadInstruction() {}

String LoadInstruction::toString() const
{
	return utils::toString("load [", data->toString(), "]", attributesToString(" (", ")"));
}

CreateVarInstruction::CreateVarInstruction(ModuleLoc loc, StringRef name)
	: Instruction(loc, Instructions::CREATEVAR), name(name)
{}
CreateVarInstruction::~CreateVarInstruction() {}

String CreateVarInstruction::toString() const
{
	return utils::toString("createVar [", name, "]", attributesToString(" (", ")"));
}

} // namespace sc