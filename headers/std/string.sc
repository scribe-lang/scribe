/*
 * This is the string type - used by everything string in standard library,
 * and is meant to be the de-facto implementation for string usage
 */

let c = @import("std/c");
let utils = @import("std/utils");

let float_precision = 3;

let setPrecision = fn(digits: const i32) {
	float_precision = digits;
};

let getPrecision = fn(): i32 {
	return float_precision;
};

let String = struct {
	data: *i8;
	len: u64;
	cap: u64;
};

let new = fn(): String {
	return String{nil, 0, 0};
};

let withCap = fn(cap: u64): String {
	let res = String{nil, 0, cap};
	if cap < 1 { return res; }
	res.cap = cap + 1; // for null terminator
	res.data = c.malloc(i8, res.cap);
	c.memset(@as(@ptr(void), res.data), 0, res.cap);
	return res;
};

let from = fn(data: *const i8): String {
	let count = c.strlen(data) + 1; // + 1 for null terminator
	let res = new();
	res.data = c.malloc(i8, count);
	c.memcpy(@as(@ptr(void), res.data), @as(@ptr(void), data), count);
	res.data[count - 1] = 0;
	res.len = count;
	res.cap = count;
	return res;
};

let deinit in String = fn() {
	if @as(u64, self.data) == nil { return; }
	c.free(i8, self.data);
	self.len = 0;
	self.cap = 0;
};

let getBuf in String = fn(): *i8 {
	return self.data;
};

let cStr in String = fn(): *const i8 {
	if @as(u64, self.data) == nil { return ""; }
	return self.data;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// str() Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let iToStr = fn(comptime T: type, data: &const T): String {
	let len = utils.countDigits(data);
	let res = withCap(len);
	// + 1 because snprintf includes space for null terminator
	// which is also taken care of by withCap()
	c.snprintf(res.getBuf(), len + 1, "%lld", data);
	return res;
};

let fToStr = fn(comptime T: type, data: &const T): String {
	let len = utils.countDigits(data) + getPrecision() + 1; // + 1 for decimal point
	let res = withCap(len);
	// + 1 because snprintf includes space for null terminator
	// which is also taken care of by withCap()
	c.snprintf(res.getBuf(), len + 1, "%.*lf", getPrecision(), data);
	return res;
};

let str in const i1 = fn(): String {
	if self == true { return from("true"); }
	return from("false");
};
let str in const i8 = fn(): String { return iToStr(i8, self); };
let str in const i16 = fn(): String { return iToStr(i16, self); };
let str in const i32 = fn(): String { return iToStr(i32, self); };
let str in const i64 = fn(): String { return iToStr(i64, self); };

let str in const u8 = fn(): String { return iToStr(u8, self); };
let str in const u16 = fn(): String { return iToStr(u16, self); };
let str in const u32 = fn(): String { return iToStr(u32, self); };
let str in const u64 = fn(): String { return iToStr(u64, self); };

let str in const f32 = fn(): String { return fToStr(f32, self); };
let str in const f64 = fn(): String { return fToStr(f64, self); };

///////////////////////////////////////////////////////////////////////////////////////////////////
// Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

inline if @isMainSrc() {

let main = fn(): i32 {
	let str = from("hi there");
	let str2 = new();
	defer str.deinit();
	defer str2.deinit();
	return 0;
};

}