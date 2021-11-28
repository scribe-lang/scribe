let c = @import("std/c");
let string = @import("std/string");

let Vec = struct<T> {
	capacity: u64;
	length: u64;
	data: *T;
	managed: i1;
};

// a function with a comptime argument is guaranteed to be specialized
let new = fn(comptime T: type, managed: i1): Vec(T) {
	return Vec(T){0, 0, nil, managed};
};

// a function inside a struct which has at least one field of type 'type' has to be specialized (generic)
let push in Vec = fn(d: &const self.T) {
	if self.capacity == 0 {
		self.capacity = 1;
		self.data = c.malloc(self.T, 1);
	} elif self.length >= self.capacity {
		self.capacity *= 2;
		self.data = c.realloc(self.T, self.data, self.capacity);
	}
	self.data[self.length++] = d;
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

let len in Vec = fn(): u64 {
	return self.length;
};

let cap in Vec = fn(): u64 {
	return self.capacity;
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

let str in const Vec = fn(): string.String {
	let res = string.from("[");
	for let i = 0; i < self.length; ++i {
		let tmp = self.data[i].str();
		defer tmp.deinit();
		res += tmp;
		if i < self.length - 1 { res.appendCStr(", "); }
	}
	res.appendCStr("]");
	return res;
};

// usage
inline if @isMainSrc() {

let main = fn(): i32 {
	let v = new(i32, true);
	defer v.deinit();
	v.push(5);
	v.pop();
	return 0;
};

}