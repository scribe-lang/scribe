let io = @import("std/io");

let main = fn(argc: i32, argv: **const u8): i32 {
	io.println("hi there");
	return 0;
};