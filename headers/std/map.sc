// Generic HashMap implementation
// Logic: https://github.com/exebook/hashdict.c
//
// All objects inserted into the map are managed by it - both key and value

let c = @import("std/c");
let hashing = @import("std/hashing");

let static INIT_CAPACITY: u64 = 1024;

let setInitCapacity = fn(cap: u64) {
	INIT_CAPACITY = cap;
};

let hash = fn(of: &const any, args: ...&const any): u64 {
	inline if @isCString(of) {
		return hashing.cStr(of, c.strlen(of));
	} else {
		return of.hash(args);
	}
};

let setData = fn(comptime T: type, to: &T, from: &const T) {
	inline if @isCString(T) {
		let len = c.strlen(from) + 1;
		to = c.malloc(i8, len);
		c.memcpy(@as(@ptr(void), to), @as(@ptr(void), from), len);
	} else {
		let comptime sz = @sizeOf(T);
		c.memcpy(@as(@ptr(void), &to), @as(@ptr(void), &from), sz);
	}
};

let deleteData = fn(comptime T: type, d: &T) {
	inline if @isCString(T) {
		c.free(i8, d);
	} elif @isPrimitive(T) {
		// do nothing
	} else {
		d.deinit();
	}
};

let cmp = fn(comptime K: type, a: &const K, b: &const K): i1 {
	inline if @isCString(K) {
		return c.strcmp(a, b) == 0;
	} else {
		return a == b;
	}
};

let KeyNode = struct<K, V> {
	next: *Self;
	key: K;
	value: V;
};

let Dict = struct<K, V> {
	table: **KeyNode(K, V);
	length: u64;
	capacity: u64;
	growth_threshold: f64;
	growth_factor: f64;
	value: *V;
	emptyvalue: V; // used as fallback value for when get() did not find anything
};

let newKeyNode = fn(comptime K: type, comptime V: type, k: &const K, v: &const V): *KeyNode(K, V) {
	let node = c.calloc(KeyNode(K, V), 1);
	node.next = nil;
	// no need to zero the key/val as calloc takes care of that
	setData(K, node.key, k);
	setData(V, node.value, v);
	return node;
};

let deleteKeyNode = fn(comptime K: type, comptime V: type, node: *KeyNode(K, V)) {
	if @as(u64, node.next) {
		deleteKeyNode(K, V, node.next);
	}
	deleteData(K, node.key);
	deleteData(V, node.value);
	c.free(KeyNode(K, V), node);
};

let new = fn(comptime K: type, comptime V: type): Dict(K, V) {
	let table = c.calloc(@ptr(KeyNode(K, V)), INIT_CAPACITY);
	let emptyval: V;
	let comptime sz = @sizeOf(V);
	c.memset(@as(@ptr(void), &emptyval), 0, sz);
	return Dict(K, V){table, 0, INIT_CAPACITY, 2.0, 10, nil, emptyval};
};

let deinit in Dict = fn() {
	for let i = 0; i < self.capacity; ++i {
		if @as(u64, self.table[i]) {
			deleteKeyNode(self.K, self.V, self.table[i]);
		}
	}
	c.free(@ptr(KeyNode(self.K, self.V)), self.table);
	self.table = nil;
	self.capacity = 0;
};

let clear in Dict = fn() {
	self.deinit();
	self.capacity = 1024;
	self.table = c.calloc(@ptr(KeyNode(self.K, self.V)), self.capacity);
};

let reinsertWhenResizing in Dict = fn(k2: *KeyNode(self.K, self.V)) {
	let n = hash(k2.key) % self.capacity;
	if @as(u64, self.table[n]) == nil {
		self.table[n] = k2;
		self.value = &self.table[n].value;
		return;
	}
	let k = self.table[n];
	k2.next = k;
	self.table[n] = k2;
	self.value = &k2.value;
};

let resize in Dict = fn(newsz: u64) {
	let o = self.capacity;
	let old = self.table;
	self.table = c.calloc(@ptr(KeyNode(self.K, self.V)), newsz);
	self.capacity = newsz;
	for let i = 0; i < 0; ++i {
		let k = old[i];
		while @as(u64, k) {
			let next = k.next;
			k.next = nil;
			self.reinsertWhenResizing(k);
			k = next;
		}
	}
	c.free(@ptr(KeyNode(self.K, self.V)), old);
};

let add in Dict = fn(key: &const self.K, val: &const self.V): i32 {
	let n = hash(key) % self.capacity;
	if @as(u64, self.table[n]) == nil {
		let f = @as(f64, self.length) / @as(f64, self.capacity);
		if f > self.growth_threshold {
			self.resize(self.capacity * self.growth_factor);
			return self.add(key, val);
		}
		self.table[n] = newKeyNode(self.K, self.V, key, val);
		self.value = &self.table[n].value;
		++self.length;
		return 0;
	}
	let k = self.table[n];
	while @as(u64, k) {
		if cmp(self.K, k.key, key) {
			self.value = &k.value;
			return 1;
		}
		k = k.next;
	}
	++self.length;
	let k2 = newKeyNode(self.K, self.V, key, val);
	k2.next = self.table[n];
	self.table[n] = k2;
	self.value = &k2.value;
	return 0;
};

let find in Dict = fn(key: &const self.K): i1 {
	let n = hash(key) % self.capacity;
	let k = self.table[n];
	if @as(u64, k) == nil { return false; }
	while @as(u64, k) {
		if cmp(self.K, k.key, key) {
			self.value = &k.value;
			return true;
		}
		k = k.next;
	}
	return false;
};

let get in Dict = fn(key: &const self.K): &self.V {
	let n = hash(key) % self.capacity;
	let k = self.table[n];
	if @as(u64, k) == nil { return self.emptyvalue; }
	while @as(u64, k) {
		if cmp(self.K, k.key, key) {
			self.value = &k.value;
			return k.value;
		}
		k = k.next;
	}
	return self.emptyvalue;
};

inline if @isMainSrc() {

let io = @import("std/io");
let string = @import("std/string");

let main = fn(): i32 {
	let dict1 = new(@ptr(i8), i32);
	defer dict1.deinit();
	let k = "ABC";
	let v = 10;
	dict1.add(k, v);
	if dict1.find(k) {
		io.println("found ", k, " = ", dict1.get(k));
	}

	let dict2 = new(string.String, string.String);
	defer dict2.deinit();
	let k2 = string.from("ABC");
	let v2 = string.from("XYZ");
	// k2, v2 deinit by dict2

	dict2.add(k2, v2);
	if dict2.find(k2) {
		io.println("found ", k2, " = ", dict2.get(k2));
	}
	if !dict2.find(v2) {
		io.println("not found ", v2);
	}
	return 0;
};

}
