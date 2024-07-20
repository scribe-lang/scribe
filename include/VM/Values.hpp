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
	ModuleLoc loc;
	Values valty;
	StringMap<String> attrs;

public:
	Value(ModuleLoc loc, Values valty);
	virtual ~Value();

	virtual String toString() const = 0;

#define isValueX(X, ENUMVAL) \
	inline bool is##X() { return valty == Values::ENUMVAL; }
	isValueX(Simple, SIMPLE);

	inline const ModuleLoc &getLoc() const { return loc; }

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

template<typename T> T *as(Value *data) { return static_cast<T *>(data); }

enum class SimpleValues
{
	NIL,
	TRUE,
	FALSE,
	INT,
	FLT,
	CHAR,
	STR,
	IDEN,
};
class SimpleValue : public Value
{
	SimpleValues simplevalty;
	Variant<String, int64_t, long double> data;

public:
	SimpleValue(ModuleLoc loc, SimpleValues simplevalty);
	SimpleValue(ModuleLoc loc, SimpleValues simplevalty, char data);
	SimpleValue(ModuleLoc loc, SimpleValues simplevalty, int64_t data);
	SimpleValue(ModuleLoc loc, SimpleValues simplevalty, long double data);
	SimpleValue(ModuleLoc loc, SimpleValues simplevalty, StringRef data);
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

	inline StringRef getDataStr() const { return std::get<String>(data); }
	inline int64_t getDataInt() const { return std::get<int64_t>(data); }
	inline long double getDataFlt() const { return std::get<long double>(data); }
};

} // namespace sc