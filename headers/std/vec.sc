let c = @import("std/c");
let iter = @import("std/iter");

let Vec = struct<T> {
	capacity: u64;
	length: u64;
	data: *T;
	managed: i1;
};

let deinit in Vec = fn() {
	defer c.free(self.T, self.data);
	if !self.managed || @isPrimitive(self.T) { return; }
	for let i: u64 = 0; i < self.length; ++i {
		inline if !@isPrimitive(self.T) {
			self.data[i].deinit();
		}
	}
};

// a function with a comptime argument is guaranteed to be specialized
let new = fn(comptime T: type, managed: i1): Vec(T) {
	return Vec(T){0, 0, nil, managed};
};

// a function inside a struct which has at least one field of type 'type' has to be specialized (generic)
// in return type, since Vec(self.T) would be self referencing, it must not be used
let push in Vec = fn(d: &const self.T): self {
	let comptime sz = @sizeOf(self.T);
	if self.capacity == 0 {
		self.capacity = 1;
		self.data = c.malloc(self.T, 1);
	} elif self.length >= self.capacity {
		self.capacity *= 2;
		self.data = c.realloc(self.T, self.data, self.capacity);
	}
	c.memcpy(@as(@ptr(void), &self.data[self.length++]), @as(@ptr(void), &d), sz);
	return self;
};

let pop in Vec = fn() {
	if self.length == 0 {
		// throw "vec.pop() in an empty vector";
		return;
	}
	--self.length;
	if !self.managed {
		return;
	}
	inline if !@isPrimitive(self.T) {
		self.data[self.length].deinit();
	}
};

let __assn__ in Vec = fn(other: &const self): &self {
	c.memcpy(@as(@ptr(void), &self), @as(@ptr(void), &other), @sizeOf(self));
	return self;
};

let __subscr__ in Vec = fn(idx: u64): &self.T {
	return self.data[idx];
};

// returns a non reference copy of the data
let getByVal in Vec = fn(idx: u64): self.T {
	return self.data[idx];
};

let back in Vec = fn(): &self.T {
	return self.data[self.length - 1];
};

let backByVal in Vec = fn(): self.T {
	return self.data[self.length - 1];
};

let len in Vec = fn(): u64 {
	return self.length;
};

let cap in Vec = fn(): u64 {
	return self.capacity;
};

let isEmpty in Vec = fn(): i1 {
	return self.length == 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Iteration Utils
///////////////////////////////////////////////////////////////////////////////////////////////////

let at in Vec = fn(idx: u64): &self.T {
	return self.data[idx];
};

let each in Vec = fn(): iter.Iter(self, u64) {
	return iter.Iter(self, u64){self, 0, self.length, 1};
};

let eachRev in Vec = fn(): iter.Iter(self, u64) {
	return iter.Iter(self, u64){self, self.length - 1, -1, -1};
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Usage
///////////////////////////////////////////////////////////////////////////////////////////////////

inline if @isMainSrc() {

let main = fn(): i32 {
	let v = new(i32, true);
	defer v.deinit();
	v.push(5);
	v.pop();
	return 0;
};

}