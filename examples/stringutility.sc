let io = @import("std/io");
let string = @import("std/string");

let main = fn(): i32 {
	let s = string.from("Hello World");
	defer s.deinit();

	io.println("In     : '", s, "'");
	io.println("- '--' : ", s.find(ref"--"));
	io.println("- 'H'  : ", s.find(ref"H"));
	io.println("- ' '  : ", s.find(ref" "));
	io.println("- 'ld' : ", s.find(ref"ld"));

	let r = s.subRef(1, 6);
	io.println("\nIn     : '", r, "'");
	io.println("- '--' : ", r.find(ref"--"));
	io.println("- 'H'  : ", r.find(ref"H"));
	io.println("- ' '  : ", r.find(ref" "));
	io.println("- 'W'  : ", r.find(ref"W"));
	return 0;
};
