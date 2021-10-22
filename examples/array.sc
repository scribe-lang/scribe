let global i32 = @createIntType(32, true);
let global i64 = @createIntType(64, true);

let comptime arr = @array(i32, 5, 5);

let comptime p = arr[2][1];

/*
let io = @import("std/io");

let main = fn(): i32 {
	let a: *i32 = @array(i32, 10);
	for let i = 0; i < 10; ++i {
		a[i] = i * i;
	}
	for let i = 0; i < 10; ++i {
		io.println(a[i]);
	}
	return 0;
};
*/