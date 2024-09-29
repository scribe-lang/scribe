#pragma once

#include "Values.hpp"

namespace sc
{

enum class Instructions
{
	LOAD,
	CREATEVAR,
	ATTRIBUTE,
};
class Instruction : public Value
{
	Instructions instrty;

public:
	Instruction(ModuleLoc loc, Instructions instrty);
	virtual ~Instruction();

#define isInstrX(X, ENUMVAL) \
	inline bool is##X() { return instrty == Instructions::ENUMVAL; }
	isInstrX(Load, LOAD);
	isInstrX(CreateVar, CREATEVAR);
	isInstrX(Attribute, ATTRIBUTE);
};

class LoadInstruction : public Instruction
{
	SimpleValue *data;

public:
	LoadInstruction(ModuleLoc loc, SimpleValue *data);
	~LoadInstruction();

	String toString() const override;

	static inline LoadInstruction *create(Allocator &allocator, ModuleLoc loc,
					      SimpleValue *data)
	{
		return allocator.alloc<LoadInstruction>(loc, data);
	}

	inline SimpleValue *getData() const { return data; }
};

// Attributes:
//   inType, type, val, comptime, static, constant, volatile, variadic, reference
class CreateVarInstruction : public Instruction
{
	String name;

public:
	CreateVarInstruction(ModuleLoc loc, StringRef name);
	~CreateVarInstruction();

	String toString() const override;

	static inline CreateVarInstruction *create(Allocator &allocator, ModuleLoc loc,
						   StringRef name)
	{
		return allocator.alloc<CreateVarInstruction>(loc, name);
	}

	inline StringRef getName() const { return name; }
};

class AttributeInstruction : public Instruction
{
	String name; // name of the attribute

public:
	AttributeInstruction(ModuleLoc loc, StringRef name);
	~AttributeInstruction();

	String toString() const override;

	static inline AttributeInstruction *create(Allocator &allocator, ModuleLoc loc,
						   StringRef name)
	{
		return allocator.alloc<AttributeInstruction>(loc, name);
	}

	inline StringRef getName() const { return name; }
};

} // namespace sc