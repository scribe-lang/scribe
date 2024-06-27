#pragma once

#include "Core.hpp"

namespace sc
{
enum class Values
{
	CONSTINT,
	CONSTFLT,
	CONSTARR,
	INSTR,
	BASICBLOCK,
	FUNCTION,
};

class Value
{
	Values vty;

protected:
	StringMap<String> attributes;

public:
	Value(Values vty);
	virtual ~Value();

#define isValueX(X, ENUMVAL) \
	inline bool is##X() { return vty == Values::ENUMVAL; }
	isValueX(ConstInt, CONSTINT);
	isValueX(ConstFlt, CONSTFLT);
	isValueX(ConstArr, CONSTARR);
	isValueX(Instr, INSTR);
	isValueX(BasicBlock, BASICBLOCK);
	isValueX(Function, FUNCTION);

	inline void setAttributes(StringMap<String> &&attrs)
	{
		using namespace std;
		swap(attributes, attrs);
	}
	inline void addAttribute(StringRef name, StringRef val = "")
	{
		attributes[String(name)] = val;
	}

	inline bool hasAttribute(StringRef name)
	{
		return attributes.find(name) != attributes.end();
	}
	inline const StringMap<String> &getAttributes() { return attributes; }
};

// Base for globals (maybe), ints, floats, etc.
// const strings are arrays of Constant
class Constant : public Value
{
public:
	Constant(Values vty);
	virtual ~Constant();
};

class ConstantInt : public Constant
{
	i64 data;

public:
	ConstantInt(Type *ty, i64 data);
	~ConstantInt();
};

class ConstantFlt : public Constant
{
	f64 data;

public:
	ConstantFlt(Type *ty, f64 data);
	~ConstantFlt();
};

// always stored globally
class ConstantArray : public Constant
{
	String data;
	size_t perelementbytes;
	// int vs float decided based on ty->isInt()

public:
	ConstantArray(Type *ty, StringRef data);
	~ConstantArray();

	// Ensure that whatever data you have (int/flt), it is correctly encoded into a string
	static ConstantArray *get(Type *ty, StringRef data);

	inline StringRef getRaw() { return data; }
};

class Instruction;
class Function;
class BasicBlock : public Value
{
	String name;
	Function *parent;
	Vector<Instruction *> instrs;

public:
	BasicBlock(StringRef name, Function *parent);
	BasicBlock(StringRef name, Function *parent, Vector<Instruction *> &&instrs);
	BasicBlock(StringRef name, Function *parent, Span<Instruction *const> instrs);
	~BasicBlock();

	inline void setInstructions(Span<Instruction *const> _instrs)
	{
		instrs.assign(_instrs.begin(), _instrs.end());
	}
	inline void setInstructions(Vector<Instruction *> &&_instrs)
	{
		using namespace std;
		swap(instrs, _instrs);
	}

	inline void addInstruction(Instruction *instr) { instrs.push_back(instr); }
	inline void setInstruction(size_t idx, Instruction *instr) { instrs[idx] = instr; }

	inline Span<Instruction *const> getInstruction() const { return instrs; }
	inline Instruction *const getInstruction(size_t idx) const { return instrs[idx]; }

	inline void clearInstructions() { instrs.clear(); }
	inline void reserveInstructions(size_t count) { instrs.reserve(count); }
};

class Function : public Value
{
	String name;
	bool is_instrinsic;
	// first basicblock in body is ret, next is args, then the actual body
	Vector<BasicBlock *> body;

public:
	Function(FuncTy *ty, StringRef name, bool is_intrinsic);
	~Function();

	BasicBlock *appendBasicBlock();
};
} // namespace sc