let io = @import("std/io");

let facto = fn(comptime T: type, data: T): T {
	if data == 1 { return 1; }
	return data + facto(T, data - 1);
};

let main = fn(): i32 {
	// TODO: make this work
	// let comptime n = facto(i32, 10);
	io.println("Factorial is: ", facto(i32, 5));
	return 0;
};