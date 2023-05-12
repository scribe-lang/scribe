let c = @import("std/c");
let mem = @import("std/mem");
let iter = @import("std/iter");

let Vec = struct<T> {
	capacity: u64;
	length: u64;
	data: *T;
	managed: i1;
};

let init in Vec = fn(managed: i1) {
	self.capacity = 0;
	self.length = 0;
	self.data = nil;
	self.managed = managed;
};
let deinit in Vec = fn() {
	defer mem.free(self.T, self.data);
	if !self.managed || @isPrimitive(self.T) { return; }
	for let i: u64 = 0; i < self.length; ++i {
		inline if !@isPrimitiveOrPtr(self.T) {
			self.data[i].deinit();
		}
	}
};

// a function with a comptime argument is guaranteed to be specialized
let new = inline fn(comptime T: type, managed: i1): Vec(T) {
	return Vec(T){0, 0, nil, managed};
};

let reserve in Vec = fn(sz: u64): self {
	if self.capacity >= sz { return self; }
	if self.capacity == 0 {
		self.data = mem.alloc(self.T, sz);
	} else {
		self.data = mem.realloc(self.T, self.data, sz);
	}
	self.capacity = sz;
	return self;
};

// a function inside a struct which has at least one field of type 'type' has to be specialized (generic)
// in return type, since Vec(self.T) would be self referencing, it must not be used
let push in Vec = fn(d: &const self.T): self {
	let comptime sz = @sizeOf(self.T);
	if self.capacity == 0 {
		self.capacity = 1;
		self.data = mem.alloc(self.T, 1);
	} elif self.length >= self.capacity {
		self.capacity *= 2;
		self.data = mem.realloc(self.T, self.data, self.capacity);
	}
	mem.cpy(&self.data[self.length++], &d, sz);
	return self;
};

let pushVal in Vec = inline fn(d: self.T): self {
	return self.push(d);
};

let insert in Vec = fn(d: &const self.T, idx: u64): self {
	if idx >= self.length { return self.push(d); }
	if self.length >= self.capacity {
		self.capacity *= 2;
		self.data = mem.realloc(self.T, self.data, self.capacity);
	}
	let comptime sz = @sizeOf(self.T);
	for let i: u64 = self.length; i > idx; --i {
		mem.cpy(&self.data[i], &self.data[i - 1], sz);
	}
	mem.cpy(&self.data[idx], &d, sz);
	++self.length;
	return self;
};

let insertVal in Vec = inline fn(d: self.T, idx: u64): self {
	return self.insert(d, idx);
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

let clear in Vec = fn() {
	let l = self.length;
	self.length = 0;
	if !self.managed || @isPrimitive(self.T) {
		mem.set(self.data, 0, @sizeOf(self.T) * self.capacity);
		return;
	}
	for let i: u64 = 0; i < l; ++i {
		inline if !@isPrimitive(self.T) {
			self.data[i].deinit();
		}
	}
	mem.set(self.data, 0, @sizeOf(self.T) * self.capacity);
};

let setManaged in Vec = fn(managed: i1): self {
	self.managed = managed;
	return self;
};

let __assn__ in Vec = fn(other: &const self): &self {
	mem.cpy(&self, &other, @sizeOf(self));
	return self;
};

let __subscr__ in Vec = inline fn(idx: u64): &self.T {
	return self.data[idx];
};

let doEach in Vec = fn(cb: any, args: ...&any) {
	for let i: u64 = 0; i < self.length; ++i {
		cb(self.data[i], args);
	}
};

// returns a non reference copy of the data
let getByVal in const Vec = inline fn(idx: u64): self.T {
	return self.data[idx];
};

let back in Vec = inline fn(): &self.T {
	return self.data[self.length - 1];
};

let backByVal in const Vec = inline fn(): self.T {
	return self.data[self.length - 1];
};

let len in const Vec = inline fn(): u64 {
	return self.length;
};

let cap in const Vec = inline fn(): u64 {
	return self.capacity;
};

let isEmpty in const Vec = inline fn(): i1 {
	return self.length == 0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Iteration Utils
///////////////////////////////////////////////////////////////////////////////////////////////////

let at in Vec = inline fn(idx: u64): &self.T {
	return self.data[idx];
};

let each in Vec = inline fn(): iter.Iter(self, u64) {
	return iter.Iter(self, u64){self, 0, self.length, 1};
};

let eachRev in Vec = inline fn(): iter.Iter(self, u64) {
	return iter.Iter(self, u64){self, self.length - 1, -1, -1};
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// SmoothSort Implementation; Vec.sort(cmp: fn(a: &const self.T, b: &const self.T): i32)
// Source: https://git.musl-libc.org/cgit/musl/tree/src/stdlib/qsort.c
///////////////////////////////////////////////////////////////////////////////////////////////////

let a_ctz_l = fn(x: u64): i32 {
	if x == 0 { return 0; }
	let count = 0;
	let bit: u64 = 1;
	while (x & bit) == 0 {
		bit <<= 1;
		++count;
	}
	return count;
};

let pntz = fn(p: @array(u64, 2)): i32 {
	let r = a_ctz_l(p[0] - 1);
	if r != 0 || (r = 8 * @sizeOf(u64) + a_ctz_l(p[1])) != 8 * @sizeOf(u64) {
		return r;
	}
	return 0;
};

let cycle = fn(width: u64, ar: **u8, n: i32) {
	let tmp: @array(u8, 256);
	let comptime sztmp = @sizeOf(tmp);
	let l: u64;

	if n < 2 { return; }

	ar[n] = tmp;
	while width {
		if sztmp < width { l = sztmp; }
		else { l = width; }
		mem.cpy(ar[n], ar[0], l);
		for let i = 0; i < n; ++i {
			mem.cpy(ar[i], ar[i + 1], l);
			ar[i] = @as(u64, ar[i]) + l;
		}
		width -= l;
	}
};

let shl = fn(p: @array(u64, 2), n: i32) {
	if n >= 8 * @sizeOf(u64) {
		n -= 8 * @sizeOf(u64);
		p[1] = p[0];
		p[0] = 0;
	}
	p[1] <<= n;
	p[1] |= p[0] >> (@sizeOf(u64) * 8 - n);
	p[0] <<= n;
};

let shr = fn(p: @array(u64, 2), n: i32) {
	if n >= 8 * @sizeOf(u64) {
		n -= 8 * @sizeOf(u64);
		p[0] = p[1];
		p[1] = 0;
	}
	p[0] >>= n;
	p[0] |= p[1] << (@sizeOf(u64) * 8 - n);
	p[1] >>= n;
};

let sift = fn(comptime T: type, head: *u8, width: u64, cmp: fn(a: &const T, b: &const T): i32, pshift: i32, lp: *u64) {
	let rt: *u8, lf: *u8;
	let ar: @array(*u8, 14 * @sizeOf(u64) + 1);
	let i = 1;
	ar[0] = head;
	while pshift > 1 {
		rt = @as(u64, head) - width;
		lf = @as(u64, head) - width - lp[pshift - 2];
		let ar0tmp = @as(@ptr(T), ar[0]);
		let rttmp = @as(@ptr(T), rt);
		let lftmp = @as(@ptr(T), lf);
		if cmp(*ar0tmp, *lftmp) >= 0 && cmp(*ar0tmp, *rttmp) >= 0 {
			break;
		}
		if cmp(*lftmp, *rttmp) >= 0 {
			ar[i++] = lf;
			head = lf;
			pshift -= 1;
		} else {
			ar[i++] = rt;
			head = rt;
			pshift -= 2;
		}
	}
	cycle(width, ar, i);
};

let trinkle = fn(comptime T: type, head: *u8, width: u64, cmp: fn(a: &const T, b: &const T): i32,
		 pp: @array(u64, 2), pshift: i32, trusty: i32, lp: *u64) {
	let stepson: *u8, rt: *u8, lf: *u8;
	let p: @array(u64, 2);
	let ar: @array(*u8, 14 * @sizeOf(u64) + 1);
	let i = 1;
	let trail: i32;

	p[0] = pp[0];
	p[1] = pp[1];

	ar[0] = head;

	while p[0] != 1 || p[1] != 0 {
		stepson = @as(u64, head) - lp[pshift];
		let stepsontmp = @as(@ptr(T), stepson);
		let ar0tmp = @as(@ptr(T), ar[0]);
		if cmp(*stepsontmp, *ar0tmp) <= 0 { break; }
		if !trusty && pshift > 1 {
			rt = @as(u64, head) - width;
			lf = @as(u64, head) - width - lp[pshift - 2];
			let rttmp = @as(@ptr(T), rt);
			let lftmp = @as(@ptr(T), lf);
			if cmp(*rttmp, *stepsontmp) >= 0 || cmp(*lftmp, *stepsontmp) >= 0 {
				break;
			}
		}

		ar[i++] = stepson;
		head = stepson;
		trail = pntz(p);
		shr(p, trail);
		pshift += trail;
		trusty = 0;
	}
	if !trusty {
		cycle(width, ar, i);
		sift(T, head, width, cmp, pshift, lp);
	}
};

let sort in Vec = fn(cmp: fn(a: &const self.T, b: &const self.T): i32) {
	let comptime width = @sizeOf(self.T);
	let lp: @array(u64, 12 * @sizeOf(u64));
	let size: u64 = width * self.length;
	let head: *u8, high: *u8;
	let p: @array(u64, 2);
	p[0] = 1;
	p[1] = 0;
	let pshift = 1;
	let trail: i32;

	if !size { return; }

	head = @as(@ptr(u8), self.data);
	high = @as(u64, head) + size - width;

	lp[0] = lp[1] = width;
	for let i: u64 = 2; (lp[i] = lp[i - 2] + lp[i - 1] + width) < size; ++i {}

	while @as(u64, head) < @as(u64, high) {
		if (p[0] & 3) == 3 {
			sift(self.T, head, width, cmp, pshift, lp);
			shr(p, 2);
			pshift += 2;
		} else {
			if lp[pshift - 1] >= (@as(u64, high) - @as(u64, head)) {
				trinkle(self.T, head, width, cmp, p, pshift, 0, lp);
			} else {
				sift(self.T, head, width, cmp, pshift, lp);
			}

			if pshift == 1 {
				shl(p, 1);
				pshift = 0;
			} else {
				shl(p, pshift - 1);
				pshift = 1;
			}
		}
		p[0] |= 1;
		head = @as(u64, head) + width;
	}

	trinkle(self.T, head, width, cmp, p, pshift, 0, lp);

	while pshift != 1 || p[0] != 1 || p[1] != 0 {
		if pshift <= 1 {
			trail = pntz(p);
			shr(p, trail);
			pshift += trail;
		} else {
			shl(p, 2);
			pshift -= 2;
			p[0] ^= 7;
			shr(p, 1);
			trinkle(self.T, @as(u64, head) - lp[pshift] - width, width, cmp, p, pshift + 1, 1, lp);
			shl(p, 1);
			p[0] |= 1;
			trinkle(self.T, @as(u64, head) - width, width, cmp, p, pshift, 1, lp);
		}
		head = @as(u64, head) - width;
	}
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