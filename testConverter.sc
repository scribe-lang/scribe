// A converter to auto convert data between types.
// If there is a type mismatch, the compiler must try to see if the data can be converted.
// Only failing if it cannot be.
#autoConverter
let global cStrToStrRef = inline fn(data: *const i8): StringRef {
	return StringRef{data, string.cstrlen(data)};
};