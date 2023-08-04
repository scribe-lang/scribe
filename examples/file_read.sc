let io = @import("std/io");
let string = @import("std/string");

let main = fn(): i32 {
	let f = io.fopen(r"examples/io.sc", r"r");
	defer f.close();
	let str = string.new();
	defer str.deinit();
	while f.read(str) >= 0 {
		if str.isEmpty() { continue; }
		io.print(str);
	}
	io.println();
	return 0;
};