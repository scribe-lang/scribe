#include "Values.hpp"

namespace sc
{

Value::Value(ModuleLoc loc, Values valty) : loc(loc), valty(valty) {}
Value::~Value() {}

StringRef Value::getAttributeValue(StringRef name) const
{
	auto loc = attrs.find(name);
	if(loc != attrs.end()) return loc->second;
	return "";
}
String Value::attributesToString(StringRef prefix, StringRef suffix) const
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

SimpleValue *SimpleValue::create(Allocator &allocator, ModuleLoc loc, SimpleValues simplevalty,
				 char data)
{
	return allocator.alloc<SimpleValue>(loc, simplevalty, data);
}
SimpleValue *SimpleValue::create(Allocator &allocator, ModuleLoc loc, SimpleValues simplevalty,
				 int64_t data)
{
	return allocator.alloc<SimpleValue>(loc, simplevalty, data);
}
SimpleValue *SimpleValue::create(Allocator &allocator, ModuleLoc loc, SimpleValues simplevalty,
				 long double data)
{
	return allocator.alloc<SimpleValue>(loc, simplevalty, data);
}
SimpleValue *SimpleValue::create(Allocator &allocator, ModuleLoc loc, SimpleValues simplevalty,
				 StringRef data)
{
	return allocator.alloc<SimpleValue>(loc, simplevalty, data);
}

String SimpleValue::toString() const
{
	String res;
	switch(simplevalty) {
	case SimpleValues::INT:
		res += "INT, ";
		res += utils::toString(getDataInt());
		break;
	case SimpleValues::FLT:
		res += "FLT, ";
		res += utils::toString(getDataFlt());
		break;
	case SimpleValues::CHAR:
		res += "CHAR, ";
		res += getDataStr();
		break;
	case SimpleValues::STR:
		res += "STR, ";
		res += getDataStr();
		break;
	case SimpleValues::IDEN:
		res += "IDEN, ";
		res += getDataStr();
		break;
	default: break;
	}
	return res;
}

} // namespace sc