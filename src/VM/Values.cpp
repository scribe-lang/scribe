#include "Values.hpp"

namespace sc
{

Value::Value(ModuleLoc loc, Values valty) : loc(loc), valty(valty) {}
Value::~Value() {}

StringRef Value::getAttributeValue(StringRef name)
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) return loc->second;
	return "";
}
String Value::attributesToString(StringRef prefix, StringRef suffix)
{
	if(attrs.empty()) return "";
	String res(prefix);
	for(auto &a : attrs) {
		res += a.first;
		if(!a.second.empty()) {
			res += "=";
			res += a.second;
		}
		res += ", ";
	}
	res.pop_back();
	res.pop_back();
	res += suffix;
	return res;
}

SimpleValue::SimpleValue(ModuleLoc loc, SimpleValues simplevalty)
	: Value(loc, Values::SIMPLE), simplevalty(simplevalty), data(0)
{}
SimpleValue::SimpleValue(ModuleLoc loc, SimpleValues simplevalty, char data)
	: Value(loc, Values::SIMPLE), simplevalty(simplevalty), data(data)
{}
SimpleValue::SimpleValue(ModuleLoc loc, SimpleValues simplevalty, int64_t data)
	: Value(loc, Values::SIMPLE), simplevalty(simplevalty), data(data)
{}
SimpleValue::SimpleValue(ModuleLoc loc, SimpleValues simplevalty, long double data)
	: Value(loc, Values::SIMPLE), simplevalty(simplevalty), data(data)
{}
SimpleValue::SimpleValue(ModuleLoc loc, SimpleValues simplevalty, StringRef data)
	: Value(loc, Values::SIMPLE), simplevalty(simplevalty), data(String(data))
{}
SimpleValue::~SimpleValue() {}

String SimpleValue::toString() const
{
	String res;
	switch(simplevalty) {
	case SimpleValues::NIL: res += "nil";
	case SimpleValues::TRUE: res += "true";
	case SimpleValues::FALSE: res += "false";
	case SimpleValues::INT: res += "INT, "; res += utils::toString(getDataInt());
	case SimpleValues::FLT: res += "FLT, "; res += utils::toString(getDataFlt());
	case SimpleValues::CHAR: res += "CHAR, "; res += getDataStr();
	case SimpleValues::STR: res += "STR, "; res += getDataStr();
	case SimpleValues::IDEN: res += "IDEN, "; res += getDataStr();
	default: break;
	}
	return res;
}

} // namespace sc