// this is a prelude file - it is ALWAYS imported before the invoked program

let c = @import("std/c");
let hashing = @import("std/hashing");

let comptime global STRING_NPOS: const u64 = -1;

let cStr in const StringRef = inline fn(): *const i8 {
	return self.data;
};

let len in const StringRef = inline fn(): u64 {
	return self.length;
};

let global r = inline fn(data: StringRef): *const i8 {
	return data.cStr();
};

let subRef in const StringRef = fn(start: u64, count: u64): StringRef {
	if start >= self.length { return StringRef{nil, 0}; }
	if count == STRING_NPOS { count = self.length - start; }
	elif start + count > self.length { count = self.length - start; }
	return StringRef{&self.data[start], count};
};

let subRefCStr = fn(data: *const i8, start: u64, count: u64): StringRef {
	if count == STRING_NPOS { count = c.strlen(&data[start]); }
	return StringRef{&data[start], count};
};

let getRefCStr = inline fn(data: *const i8): StringRef {
	return subRefCStr(data, 0, STRING_NPOS);
};

let global toStringRef = fn(data: &const any): StringRef {
	inline if @isCString(data) {
		return getRefCStr(data);
	} else {
		@compilerError("toStringRef() only works for C strings at the moment");
	}
};

let ref in const i8 = inline fn(): StringRef {
	return subRefCStr(&self, 0, 1);
};

let isEmpty in const StringRef = inline fn(): i1 {
	return @as(u64, self.data) == nil || self.length == 0;
};

let hash in const StringRef = inline fn(): u64 {
	return hashing.cStr(self.data, self.length);
};

let __assn__ in StringRef = fn(other: StringRef): &self {
	self.data = other.data;
	self.length = other.length;
	return self;
};

let __subscr__ in StringRef = inline fn(idx: u64): &const i8 {
	return self.data[idx];
};

let __eq__ in const StringRef = fn(other: StringRef): i1 {
	if self.length != other.length { return false; }
	if self.length == 0 { return false; }
	return c.strncmp(self.data, other.data, self.length) == 0;
};

let __ne__ in const StringRef = inline fn(other: StringRef): i1 {
	return !(self == other);
};

let find in const StringRef = fn(other: StringRef): u64 {
	let slen = self.len();
	let olen = other.len();
	if slen == 0 || olen == 0 || olen > slen { return STRING_NPOS; }
	let pos: u64 = STRING_NPOS;
	for let i: u64 = 0; i < slen; ++i {
		if self.data[i] != *other.data { continue; }
		let found = true;
		for let j: u64 = 0; j < olen; ++j {
			if i + j >= slen || self.data[i + j] != other.data[j] {
				found = false;
				break;
			}
		}
		if !found { continue; }
		pos = i;
		break;
	}
	return pos;
};

let rfind in const StringRef = fn(other: StringRef): u64 {
	let slen = self.len();
	let olen = other.len();
	if slen == 0 || olen == 0 || olen > slen { return STRING_NPOS; }
	let pos: u64 = STRING_NPOS;
	let i: u64 = slen - 1, j: u64 = olen - 1;
	let found = false;
	while i >= 0 && j >= 0 {
		if self.data[i] != other.data[j] {
			if i == 0 { break; }
			--i;
			found = false;
			continue;
		}
		found = true;
		if j == 0 { pos = i; }
		if i == 0 || j == 0 { break; }
		--i;
		--j;
	}
	return pos;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Conversion Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let int in const StringRef = fn(): i64 {
	let tmp: @array(i8, 1024);
	c.strncpy(tmp, self.data, self.length);
	tmp[self.length] = '\0';
	return c.atoll(tmp);
};
let uint in const StringRef = fn(): u64 {
	let tmp: @array(i8, 1024);
	c.strncpy(tmp, self.data, self.length);
	tmp[self.length] = '\0';
	return c.atoull(tmp);
};
let flt in const StringRef = fn(): f64 {
	let tmp: @array(i8, 1024);
	c.strncpy(tmp, self.data, self.length);
	tmp[self.length] = '\0';
	return c.atof(tmp);
};