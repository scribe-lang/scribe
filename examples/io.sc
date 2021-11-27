let io = @import("std/io");

let main = fn(): i32 {
	io.print("On first line ", 1, ' ', 2);
	io.println(" continuing on first line ", 5, ' ', 10);
	return 0;
};