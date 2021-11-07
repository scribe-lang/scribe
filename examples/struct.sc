let st = struct {
	T: type;
	data: *T;
	len: i64;
	cap: i64;
};

let f in st = fn(d: self.T, e: self.T): self.T {
	return d + e + self.len + self.cap;
};

let s = st(i32, nil, 2, 4);

let comptime res = s.f(1, 2); // should be 9