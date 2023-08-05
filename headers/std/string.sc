/*
 * This is the string type - used by everything string in standard library,
 * and is meant to be the de-facto implementation for string usage
 */

let c = @import("std/c");
let mem = @import("std/mem");
let vec = @import("std/vec"); // required for vec.str() and str.delim()
let utils = @import("std/utils");
let hashing = @import("std/hashing");

let comptime NPOS: const u64 = STRING_NPOS;

let float_precision = 3;

let setPrecision = inline fn(digits: const i32) {
	float_precision = digits;
};

let getPrecision = inline fn(): i32 {
	return float_precision;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// String Type
///////////////////////////////////////////////////////////////////////////////////////////////////

let String = struct {
	data: *i8;
	length: u64;
	capacity: u64;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Creation/Deletion Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let deinit in String = fn() {
	if @as(u64, self.data) == nil { return; }
	mem.free(i8, self.data);
	self.length = 0;
	self.capacity = 0;
	self.data = nil;
};

let new = inline fn(): String {
	return String{nil, 0, 0};
};

let reserve in String = fn(sz: u64): self {
	if self.capacity >= sz { return self; }
	if self.capacity == 0 {
		self.data = mem.alloc(i8, sz + 1);
	} else {
		self.data = mem.realloc(i8, self.data, sz + 1);
	}
	self.capacity = sz + 1; // for null terminator
	mem.set(self.data, 0, self.capacity);
	return self;
};

let withCap = fn(capacity: u64): String {
	let res = String{nil, 0, 0};
	return res.reserve(capacity);
};

let fromCStr = fn(data: *const i8): String {
	let count = c.strlen(data) + 1; // + 1 for null terminator
	let res = new();
	res.data = mem.alloc(i8, count);
	mem.cpy(res.data, data, count);
	res.data[count - 1] = 0;
	res.length = count - 1;
	res.capacity = count;
	return res;
};

// when using this, ensure that count < strlen(data)
let fromSubCStr = fn(data: *const i8, count: u64): String {
	count = count + 1; // + 1 for null terminator
	let res = new();
	res.data = mem.alloc(i8, count);
	mem.cpy(res.data, data, count);
	res.data[count - 1] = 0; // set null terminator at the end
	res.length = count - 1;
	res.capacity = count;
	return res;
};

let fromStringRef = inline fn(data: StringRef): String {
	return fromSubCStr(data.data, data.length);
};

// from() is implemented later with the Utility Functions

let global s = inline fn(data: *const i8): String {
	return fromCStr(data);
};

let str in const StringRef = fn(): String {
	return fromStringRef(self);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Core Utility Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let getBuf in String = inline fn(): *&i8 {
	return self.data;
};

let setLen in String = inline fn(len: u64) {
	self.length = len;
};

let cStr in const String = fn(): *const i8 {
	if @as(u64, self.data) == nil { return r""; }
	return self.data;
};

let len in const String = inline fn(): u64 {
	return self.length;
};

let cap in const String = inline fn(): u64 {
	return self.capacity;
};

let isEmpty in const String = inline fn(): i1 {
	return self.length == 0;
};

let clear in String = fn() {
	if self.length == 0 { return; }
	mem.set(self.data, 0, self.length);
	self.length = 0;
};

let copy in const String = inline fn(): String {
	return fromCStr(self.data);
};

let hash in const String = inline fn(): u64 {
	return hashing.cStr(self.data, self.length);
};

let subString in const String = fn(start: u64, count: u64): String {
	if start >= self.length { return new(); }
	return fromSubCStr(&self.data[start], count);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// StringRef related functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let ref in const String = inline fn(): StringRef {
	return StringRef{self.data, self.length};
};

let subRef in const String = inline fn(start: u64, count: u64): StringRef {
	return self.ref().subRef(start, count);
};

let isSpace in const i8 = inline fn(): i1 {
	return c.isspace(self);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let appendChar in String = fn(ch: i8): &String {
	if self.capacity == 0 {
		self.capacity = 2;
		self.data = mem.alloc(i8, self.capacity);
	} elif self.capacity < self.length + 2 {
		self.capacity = self.capacity * 2;
		self.data = mem.realloc(i8, self.data, self.capacity);
	}
	self.data[self.length++] = ch;
	self.data[self.length] = 0;
	return self;
};

let appendCStr in String = fn(other: *const i8, count: u64): &String {
	if @as(u64, other) == nil { return self; }
	if !count || count == NPOS { count = c.strlen(other); }
	if !count { return self; }
	if self.capacity == 0 {
		self.capacity = count + 1;
		self.data = mem.alloc(i8, self.capacity);
	} elif self.capacity < self.length + count + 1 {
		self.capacity = self.length + count + 1;
		self.data = mem.realloc(i8, self.data, self.capacity);
	}
	mem.cpy(&self.data[self.length], other, count);
	self.length += count;
	self.data[self.length] = 0;
	return self;
};

let appendRef in String = inline fn(other: StringRef): &String {
	return self.appendCStr(other.data, other.length);
};

let appendInt in String = fn(data: i64): &String {
	let otherlen = utils.countIntDigits(data);
	if self.capacity == 0 {
		self.capacity = otherlen + 1;
		self.data = mem.alloc(i8, self.capacity);
	} elif self.capacity < self.length + otherlen + 1 {
		self.capacity = self.length + otherlen + 1;
		self.data = mem.realloc(i8, self.data, self.capacity);
	}
	c.snprintf(&self.data[self.length], otherlen + 1, r"%lld", data);
	self.length += otherlen;
	self.data[self.length] = 0;
	return self;
};

let appendUInt in String = fn(data: u64): &String {
	let otherlen = utils.countUIntDigits(data);
	if self.capacity == 0 {
		self.capacity = otherlen + 1;
		self.data = mem.alloc(i8, self.capacity);
	} elif self.capacity < self.length + otherlen + 1 {
		self.capacity = self.length + otherlen + 1;
		self.data = mem.realloc(i8, self.data, self.capacity);
	}
	c.snprintf(&self.data[self.length], otherlen + 1, r"%llu", data);
	self.length += otherlen;
	self.data[self.length] = 0;
	return self;
};

let appendFlt in String = fn(data: f64): &String {
	let otherlen = utils.countIntDigits(data) + getPrecision() + 1;
	if self.capacity == 0 {
		self.capacity = otherlen + 1;
		self.data = mem.alloc(i8, self.capacity);
	} elif self.capacity < self.length + otherlen + 1 {
		self.capacity = self.length + otherlen + 1;
		self.data = mem.realloc(i8, self.data, self.capacity);
	}
	c.snprintf(&self.data[self.length], otherlen + 1, r"%.*lf", getPrecision(), data);
	self.length += otherlen;
	self.data[self.length] = 0;
	return self;
};

let append in String = fn(data: ...&const any): &String {
	let comptime len = @valen();
	inline for let comptime i = 0; i < len; ++i {
		inline if @isEqualTy(data[i], i8) {
			self.appendChar(data[i]);
		} elif @isCString(data[i]) {
			self.appendCStr(data[i], 0);
		} elif @isEqualTy(data[i], StringRef) {
			self.appendRef(data[i]);
		} elif @isInt(data[i]) {
			inline if @isIntSigned(data[i]) {
				self.appendInt(data[i]);
			} else {
				self.appendUInt(data[i]);
			}
		} elif @isFlt(data[i]) {
			self.appendFlt(data[i]);
		} elif @isEqualTy(data[i], String) {
			self.appendCStr(data[i].data, data[i].length);
		} else { // just attempt to convert the data to string and append that
			let s = data[i].str();
			defer s.deinit();
			self.appendCStr(s.data, s.length);
		}
	}
	return self; // this exists only to avoid warning by C compiler about non-void return
};

// Generic, variadic argument based creation function
let from = inline fn(data: ...&const any): String {
	let comptime len = @valen();
	let res = new();
	inline for let comptime i = 0; i < len; ++i {
		res.append(data[i]);
	}
	return res;
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

let __assn__ in String = fn(other: &const any): &String {
	self.clear();
	self.append(other);
	return self;
};

let __add__ in const String = fn(other: &const String): String {
	let res = withCap(self.length + other.length);
	if self.length > 0 {
		mem.cpy(res.data, self.data, self.length);
		res.length = self.length;
	}
	if other.length > 0 {
		mem.cpy(&res.data[res.length], other.data, other.length);
		res.length += other.length;
	}
	res.data[res.length] = 0;
	return res;
};

let __add_assn__ in String = inline fn(other: &const any): &String {
	return self.append(other);
};

let __subscr__ in String = inline fn(idx: u64): &i8 {
	return self.data[idx];
};

let __eq__ in const String = fn(other: &const String): i1 {
	if c.strcmp(self.cStr(), other.cStr()) == 0 { return true; }
	return false;
};

let __ne__ in const String = inline fn(other: &const String): i1 {
	return !(self == other);
};

let find in const String = inline fn(other: StringRef): u64 {
	return self.ref().find(other);
};

let rfind in const String = inline fn(other: StringRef): u64 {
	return self.ref().rfind(other);
};

let delim in const String = fn(ch: i8): vec.Vec(String) {
	let res = vec.new(String, true);
	let last = 0;
	for let i = 0; i < self.length; ++i {
		if self.data[i] == ch && i >= last {
			if i == last {
				res.push(new());
			} else {
				res.push(fromSubCStr(&self.data[last], i - last));
			}
			last = i + 1;
			continue;
		}
	}
	if self.length >= last {
		if self.length == last {
			res.push(new());
		} else {
			res.push(fromSubCStr(&self.data[last], self.length - last));
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
// Conversion Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let int in const String = inline fn(): i64 {
	return c.atoll(self.cStr());
};
let uint in const String = inline fn(): u64 {
	return c.atoull(self.cStr());
};
let flt in const String = inline fn(): f64 {
	return c.atof(self.cStr());
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// str() Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let iToStr = fn(comptime T: type, data: &const T): String {
	let length = utils.countIntDigits(data);
	let res = withCap(length);
	// + 1 because snprintf includes space for null terminator
	// which is also taken care of by withCap()
	c.snprintf(res.getBuf(), length + 1, c.getTypeSpecifier(T), data);
	return res;
};

let uToStr = fn(comptime T: type, data: &const T): String {
	let length = utils.countUIntDigits(data);
	let res = withCap(length);
	// + 1 because snprintf includes space for null terminator
	// which is also taken care of by withCap()
	c.snprintf(res.getBuf(), length + 1, c.getTypeSpecifier(T), data);
	return res;
};

let fToStr = fn(comptime T: type, data: &const T): String {
	let length = utils.countIntDigits(data) + getPrecision() + 1; // + 1 for decimal point
	let res = withCap(length);
	// + 1 because snprintf includes space for null terminator
	// which is also taken care of by withCap()
	c.snprintf(res.getBuf(), length + 1, c.getTypeSpecifier(T), getPrecision(), data);
	return res;
};

let str in const i1 = fn(): String {
	if self == true { return from("true"); }
	return from("false");
};
let str in const i8 = inline fn(): String { return iToStr(i8, self); };
let str in const i16 = inline fn(): String { return iToStr(i16, self); };
let str in const i32 = inline fn(): String { return iToStr(i32, self); };
let str in const i64 = inline fn(): String { return iToStr(i64, self); };

let str in const u8 = inline fn(): String { return uToStr(u8, self); };
let str in const u16 = inline fn(): String { return uToStr(u16, self); };
let str in const u32 = inline fn(): String { return uToStr(u32, self); };
let str in const u64 = inline fn(): String { return uToStr(u64, self); };

let str in const f32 = inline fn(): String { return fToStr(f32, self); };
let str in const f64 = inline fn(): String { return fToStr(f64, self); };

let str in const vec.Vec = fn(): String {
	let res = from("[");
	for let i = 0; i < self.length; ++i {
		res += self.data[i];
		if i < self.length - 1 { res.appendRef(", "); }
	}
	res.appendRef("]");
	return res;
};

let str in const c.ioctl.winsize = fn(): String {
	let res = from("{ws_row: ");
	res += self.ws_row;
	res += ", ws_col: ";
	res += self.ws_col;
	res += ", ws_xpixel: ";
	res += self.ws_xpixel;
	res += ", ws_ypixel: ";
	res += self.ws_ypixel;
	res += "}";
	return res;
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