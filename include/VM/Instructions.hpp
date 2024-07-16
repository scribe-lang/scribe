#pragma once

#include "Error.hpp"

namespace sc
{

enum class Values
{
	SIMPLE,
	INSTRUCTION,
};
class Value
{
	Values valty;
	ModuleLoc *loc;
	StringMap<String> attrs;

public:
	Value(const ModuleLoc *loc);
	virtual ~Value();

	virtual String toString() const = 0;

#define isValueX(X, ENUMVAL) \
	inline bool is##X() { return valty == Values::ENUMVAL; }
	isValueX(Simple, SIMPLE);

	inline bool hasAttribute(StringRef name) { return attrs.find(name) != attrs.end(); }
	inline void addAttribute(StringRef name, StringRef val = "") { attrs[String(name)] = val; }
	inline void setAttributes(const StringMap<String> &_attrs) { attrs = _attrs; }
	inline void setAttributes(StringMap<String> &&_attrs)
	{
		using namespace std;
		swap(attrs, _attrs);
	}
	StringRef getAttributeValue(StringRef name);
	String attributesToString(StringRef prefix = "", StringRef suffix = "");
};

enum class SimpleValues
{
	NIL,
	TRUE,
	FALSE,
	CHAR,
	INT,
	FLT,
	STR,
	IDEN,
};
class SimpleValue : public Value
{
	SimpleValues simplevalty;
	Variant<String, int64_t, long double> data;

public:
	SimpleValue(SimpleValues valty);
	SimpleValue(SimpleValues valty, char data);
	SimpleValue(SimpleValues valty, int64_t data);
	SimpleValue(SimpleValues valty, long double data);
	SimpleValue(SimpleValues valty, StringRef data);
	~SimpleValue();

	String toString() const override;

#define isSimpleValueX(X, ENUMVAL) \
	inline bool is##X() { return simplevalty == SimpleValues::ENUMVAL; }
	isSimpleValueX(Nil, NIL);
	isSimpleValueX(True, TRUE);
	isSimpleValueX(False, FALSE);
	isSimpleValueX(Int, INT);
	isSimpleValueX(Flt, FLT);
	isSimpleValueX(Char, CHAR);
	isSimpleValueX(Str, STR);
	isSimpleValueX(Iden, IDEN);

	inline SimpleValues getType() { return simplevalty; }

	inline StringRef getDataStr() { return std::get<String>(data); }
	inline int64_t getDataInt() { return std::get<int64_t>(data); }
	inline long double getDataFlt() { return std::get<long double>(data); }
};

enum class Instructions
{
	LOAD,
};
class Instruction : public Value
{
	Instructions instrty;
	Vector<Value *> args;

public:
	Instruction(Instructions instrty);
	virtual ~Instruction();

	inline void addArg(Value *arg) { args.push_back(arg); }
	inline Value *getArg(size_t idx) { return args[idx]; }
	inline size_t getArgCount() { return args.size(); }
	inline bool hasArgs() { return !args.empty(); }
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
};

} // namespace sc