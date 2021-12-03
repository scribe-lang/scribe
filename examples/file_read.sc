let io = @import("std/io");
let string = @import("std/string");

let main = fn(): i32 {
	let f = io.fopen("examples/io.sc", "r");
	defer f.close();
	let str = string.new();
	defer str.deinit();
	while f.read(str) >= 0 {
		if str.empty() { continue; }
		io.print(str);
	}
	io.println();
	return 0;
};