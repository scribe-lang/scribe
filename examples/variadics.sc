let global i32 = @createIntType(32, true);
let global i64 = @createIntType(64, true);

let f = fn(comptime data: ...any): i32 {
	return data[0] + data[1];
};

let f2 = fn(comptime data: ...any): i32 {
	return @valen(data);
};

let comptime s = f(5, 6);
let comptime t = f2(1, "2", 3.0);