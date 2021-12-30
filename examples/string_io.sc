let io = @import("std/io");
let string = @import("std/string");

let main = fn(): i32 {
	let str = string.from("hi there");
	let numstr = 549.str();
	let flt = 2.125;
	let fltstr = flt.str();
	defer str.deinit();
	defer numstr.deinit();
	defer fltstr.deinit();
	io.println(str);
	io.println(numstr);
	io.println(fltstr);

	// input
	let input = string.new();
	defer input.deinit();
	io.println("Enter data:");
	io.stdin.read(input);
	input.trim();
	io.println(input);
	return 0;
};