#pragma once

#include "Values.hpp"
#include "VMContext.hpp"

namespace sc
{

enum class Instructions
{
	LOAD,
};
class Instruction : public Value
{
	Instructions instrty;

protected:
	Vector<Value *> args;

public:
	Instruction(ModuleLoc loc, Instructions instrty);
	Instruction(ModuleLoc loc, Instructions instrty, size_t argscapacity);
	virtual ~Instruction();

	inline void addArg(Value *arg) { args.push_back(arg); }
	inline Value *getArg(size_t idx) const { return args[idx]; }
	inline size_t getArgCount() const { return args.size(); }
	inline bool hasArgs() const { return !args.empty(); }
	inline Vector<Value *> &getArgs() { return args; }
	inline void setArgs(const Vector<Value *> &_args) { args = _args; }
	inline void setArgs(Vector<Value *> &&_args)
	{
		using namespace std;
		swap(args, _args);
	}
};

// Args:
// Data: SimpleValue
class LoadInstruction : public Instruction
{
public:
	LoadInstruction(ModuleLoc loc, SimpleValue *data);
	~LoadInstruction();

	inline String toString() const override;

	static inline LoadInstruction *create(Context &c, ModuleLoc loc, SimpleValue *data)
	{
		return c.allocValue<LoadInstruction>(loc, data);
	}

	inline SimpleValue *getData() const { return as<SimpleValue>(getArg(0)); }
};

} // namespace sc