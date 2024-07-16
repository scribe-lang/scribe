@setMaxCompilerErrors(20);

let c = @import("std/c");
let io = @import("std/io");
let string = @import("std/string");

let main = fn(): i32 {
	let s = string.from("Hello world");
	defer s.deinit();
	io.stdout.printf("%d %d %s\n", 5, 6, s); // fails to compile
	return 0;
};
