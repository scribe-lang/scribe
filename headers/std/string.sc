/*
 * This is the string type - used by everything string in standard library,
 * and is meant to be the de-facto implementation for string usage
 */

let c = @import("std/c");
let vec = @import("std/vec"); // required for vec.str() and str.delim()
let utils = @import("std/utils");
let hashing = @import("std/hashing");

let float_precision = 3;

let setPrecision = fn(digits: const i32) {
	float_precision = digits;
};

let getPrecision = fn(): i32 {
	return float_precision;
};

let String = struct {
	data: *i8;
	length: u64;
	capacity: u64;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Creation/Deletion Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let new = fn(): String {
	return String{nil, 0, 0};
};

let withCap = fn(capacity: u64): String {
	let res = String{nil, 0, capacity};
	if capacity < 1 { return res; }
	res.capacity = capacity + 1; // for null terminator
	res.data = c.malloc(i8, res.capacity);
	c.memset(@as(@ptr(void), res.data), 0, res.capacity);
	return res;
};

let from = fn(data: *const i8): String {
	let count = c.strlen(data) + 1; // + 1 for null terminator
	let res = new();
	res.data = c.malloc(i8, count);
	c.memcpy(@as(@ptr(void), res.data), @as(@ptr(void), data), count);
	res.data[count - 1] = 0;
	res.length = count - 1;
	res.capacity = count;
	return res;
};

let fromSlice = fn(data: *const i8, count: u64): String {
	count = count + 1; // + 1 for null terminator
	let res = new();
	res.data = c.malloc(i8, count);
	c.memcpy(@as(@ptr(void), res.data), @as(@ptr(void), data), count);
	res.data[count - 1] = 0;
	res.length = count - 1;
	res.capacity = count;
	return res;
};

let init in String = fn() {
	self.data = nil;
	self.length = 0;
	self.capacity = 0;
};

let deinit in String = fn() {
	if @as(u64, self.data) == nil { return; }
	c.free(i8, self.data);
	self.length = 0;
	self.capacity = 0;
	self.data = nil;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Core Utility Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let getBuf in String = fn(): *&i8 {
	return self.data;
};

let cStr in const String = fn(): *const i8 {
	if @as(u64, self.data) == nil { return ""; }
	return self.data;
};

let len in const String = fn(): u64 {
	return self.length;
};

let cap in const String = fn(): u64 {
	return self.capacity;
};

let empty in const String = fn(): i1 {
	return self.length == 0;
};

let clear in String = fn() {
	if self.length == 0 { return; }
	c.memset(@as(@ptr(void), self.data), 0, self.length);
	self.length = 0;
};

let copy in const String = fn(): String {
	return from(self.data);
};

let hash in const String = fn(): u64 {
	return hashing.cStr(self.data, self.length);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// str() Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let iToStr = fn(comptime T: type, data: &const T): String {
	let length = utils.countIntDigits(data);
	let res = withCap(length);
	// + 1 because snprintf includes space for null terminator
	// which is also taken care of by withCap()
	c.snprintf(res.getBuf(), length + 1, "%lld", data);
	return res;
};

let uToStr = fn(comptime T: type, data: &const T): String {
	let length = utils.countUIntDigits(data);
	let res = withCap(length);
	// + 1 because snprintf includes space for null terminator
	// which is also taken care of by withCap()
	c.snprintf(res.getBuf(), length + 1, "%llu", data);
	return res;
};

let fToStr = fn(comptime T: type, data: &const T): String {
	let length = utils.countIntDigits(data) + getPrecision() + 1; // + 1 for decimal point
	let res = withCap(length);
	// + 1 because snprintf includes space for null terminator
	// which is also taken care of by withCap()
	c.snprintf(res.getBuf(), length + 1, "%.*lf", getPrecision(), data);
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

let str in const u8 = fn(): String { return uToStr(u8, self); };
let str in const u16 = fn(): String { return uToStr(u16, self); };
let str in const u32 = fn(): String { return uToStr(u32, self); };
let str in const u64 = fn(): String { return uToStr(u64, self); };

let str in const f32 = fn(): String { return fToStr(f32, self); };
let str in const f64 = fn(): String { return fToStr(f64, self); };

let str in const vec.Vec = fn(): String {
	let res = from("[");
	for let i = 0; i < self.length; ++i {
		inline if @isEqualTy(self.T, String) {
			res += self.data[i];
		} else {
			let tmp = self.data[i].str();
			defer tmp.deinit();
			res += tmp;
		}
		if i < self.length - 1 { res.appendCStr(", "); }
	}
	res.appendCStr("]");
	return res;
};

let isSpace in const i8 = fn(): i1 {
	return c.isspace(self);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let appendChar in String = fn(ch: i8): &String {
	if self.capacity == 0 {
		self.capacity = 2;
		self.data = c.malloc(i8, self.capacity);
	} elif self.capacity < self.length + 2 {
		self.capacity = self.capacity * 2;
		self.data = c.realloc(i8, self.data, self.capacity);
	}
	self.data[self.length++] = ch;
	self.data[self.length] = 0;
	return self;
};

let appendCStr in String = fn(other: *const i8): &String {
	let otherlen = c.strlen(other);
	if !otherlen { return self; }
	if self.capacity == 0 {
		self.capacity = otherlen + 1;
		self.data = c.malloc(i8, self.capacity);
	} elif self.capacity < self.length + otherlen + 1 {
		self.capacity = self.length + otherlen + 1;
		self.data = c.realloc(i8, self.data, self.capacity);
	}
	c.memcpy(@as(@ptr(void), &self.data[self.length]), @as(@ptr(void), other), otherlen + 1);
	self.length += otherlen;
	return self;
};

let appendInt in String = fn(data: i64): &String {
	let otherlen = utils.countIntDigits(data);
	if self.capacity == 0 {
		self.capacity = otherlen + 1;
		self.data = c.malloc(i8, self.capacity);
	} elif self.capacity < self.length + otherlen + 1 {
		self.capacity = self.length + otherlen + 1;
		self.data = c.realloc(i8, self.data, self.capacity);
	}
	c.snprintf(&self.data[self.length], otherlen + 1, "%lld", data);
	self.length += otherlen;
	self.data[self.length] = 0;
	return self;
};

let appendUInt in String = fn(data: i64): &String {
	let otherlen = utils.countUIntDigits(data);
	if self.capacity == 0 {
		self.capacity = otherlen + 1;
		self.data = c.malloc(i8, self.capacity);
	} elif self.capacity < self.length + otherlen + 1 {
		self.capacity = self.length + otherlen + 1;
		self.data = c.realloc(i8, self.data, self.capacity);
	}
	c.snprintf(&self.data[self.length], otherlen + 1, "%llu", data);
	self.length += otherlen;
	self.data[self.length] = 0;
	return self;
};

let appendFlt in String = fn(data: f64): &String {
	let otherlen = utils.countIntDigits(data) + getPrecision() + 1;
	if self.capacity == 0 {
		self.capacity = otherlen + 1;
		self.data = c.malloc(i8, self.capacity);
	} elif self.capacity < self.length + otherlen + 1 {
		self.capacity = self.length + otherlen + 1;
		self.data = c.realloc(i8, self.data, self.capacity);
	}
	c.snprintf(&self.data[self.length], otherlen + 1, "%.*lf", getPrecision(), data);
	self.length += otherlen;
	self.data[self.length] = 0;
	return self;
};

let append in String = fn(other: &const String): &String {
	return self.appendCStr(other.cStr());
};

let erase in String = fn(idx: u64): i1 {
	if self.length <= idx { return false; }
	for let i = idx; i < self.length; ++i {
		// this works since length = capacity - 1, always
		self.data[i] = self.data[i + 1];
	}
	--self.length;
	return true;
};

let __assn__ in String = fn(other: &const String): &String {
	self.clear();
	self.append(other);
	return self;
};

let __add__ in const String = fn(other: &const String): String {
	let res = withCap(self.length + other.length);
	if self.length > 0 {
		c.memcpy(@as(@ptr(void), res.data), @as(@ptr(void), self.data), self.length);
		res.length = self.length;
	}
	if other.length > 0 {
		c.memcpy(@as(@ptr(void), &res.data[res.length]), @as(@ptr(void), other.data), other.length);
		res.length += other.length;
	}
	res.data[res.length] = 0;
	return res;
};

let __add_assn__ in String = fn(other: &const String): &String {
	return self.append(other);
};

let __subscr__ in String = fn(idx: u64): &i8 {
	return self.data[idx];
};

let __eq__ in const String = fn(other: &const String): i1 {
	if c.strcmp(self.cStr(), other.cStr()) == 0 { return true; }
	return false;
};

let __ne__ in const String = fn(other: &const String): i1 {
	return !(self == other);
};

let delim in const String = fn(ch: i8): vec.Vec(String) {
	let res = vec.new(String, true);
	let last = 0;
	for let i = 0; i < self.length; ++i {
		if self.data[i] == ch && i >= last {
			if i == last {
				res.push(new());
			} else {
				res.push(fromSlice(&self.data[last], i - last));
			}
			last = i + 1;
			continue;
		}
	}
	if self.length >= last {
		if self.length == last {
			res.push(new());
		} else {
			res.push(fromSlice(&self.data[last], self.length - last));
		}
	}
	return res;
};

let trim in String = fn() {
	while self.length > 0 {
		if !self.data[0].isSpace() { break; }
		self.erase(0);
	}
	let i: u64 = 0;
	if self.length > 0 { i = self.length - 1; }
	while i > 0 {
		if !self.data[i].isSpace() { break; }
		self.erase(i);
		--i;
	}
};

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