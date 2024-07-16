#pragma once

#include "Error.hpp"

namespace sc
{
enum class Instructions
{
	LOAD,
};

class Value
{
	ModuleLoc *loc;
	StringMap<String> attrs;
	Vector<Value *> args;

public:
	Value(const ModuleLoc *loc);
	virtual ~Value();

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

class SimpleValue : public Value
{
	Variant<String, int64_t, long double> data;

public:
	inline bool isDataStr() { return std::holds_alternative<String>(data); }
	inline bool isDataInt() { return std::holds_alternative<String>(data); }
	inline bool isDataFlt() { return std::holds_alternative<String>(data); }
};

class Instruction
{};

} // namespace sc