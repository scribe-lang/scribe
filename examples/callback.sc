let cbfn = fn(a: i32): i32 {
	return a + 5;
};

let x = fn(comptime T: type, d: T, cb: any): T {
	return cb(d);
};

let main = fn(): i32 {
	return x(i32, 10, cbfn);
};