let io = @import("std/io");

// struct declaration
let B = struct<T>;

let A = struct {
	b: *B(i32);
};

// struct definition
let B = struct<T> {
	a: *A;
	c: T;
};

let main = fn(): i32 {
	let a = A{nil};
	let b = B(i32){nil, 1};
	a.b = &b;
	a.b.c = 10;
	io.println(a.b, " ", a.b.c);
	return 0;
};
