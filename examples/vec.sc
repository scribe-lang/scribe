let deinit in i32 = fn(): void {};

let Vec = struct<T> {
	cap: u64;
	len: u64;
	data: *T;
	managed: i1;
};

// a function with a comptime argument is guaranteed to be specialized
let new = fn(comptime T: type, managed: i1): Vec(T) {
	return Vec(T){nil, 0, 0, managed};
};

// a function inside a struct which has at least one field of type 'type' has to be specialized (generic)
let push in Vec = fn(d: &const self.T) {
	self.data[self.len++] = d;
};

let pop in Vec = fn() {
	if self.len == 0 {
		// throw "vec.pop() in an empty vector";
	}
	--self.len;
	if !self.managed {
		return;
	}
	self.data[self.len].deinit();
};

let deinit in Vec = fn() {
	// defer c.free(data);
	if !self.managed || @isPrimitive(self.T) { return; }
	for let i: u64 = 0; i < self.len; ++i {
		self.data[i].deinit();
	}
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