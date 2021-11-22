let c = @import("std/c");
let string = @import("std/string");

let main = fn(): i32 {
	let str = string.from("hi there");
	let numstr = 549.str();
	let flt = 2.125;
	let fltstr = flt.str();
	defer str.deinit();
	defer numstr.deinit();
	defer fltstr.deinit();
	c.puts(str.cStr());
	c.puts(numstr.cStr());
	c.puts(fltstr.cStr());
	return 0;
};