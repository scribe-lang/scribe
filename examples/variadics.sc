let f = fn(data: ...any): i32 {
	return data[0] + data[1];
};

let f2 = fn(data: ...any): i32 {
	return @valen();
};

let main = fn(): i32 {
	let comptime s = f(5, 6); // must be 11
	let comptime t = f2(1, "2", 3.0); // must be 3
	return 0;
};