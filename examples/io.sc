let io = @import("std/io");
let string = @import("std/string");

let main = fn(): i32 {
	let s = string.from("Hi there");
	defer s.deinit();
	io.print(s, "... On first line ", 1, ' ', 2);
	io.println(" continuing on first line ", 5, ' ', 10);
	return 0;
};