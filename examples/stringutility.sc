let io = @import("std/io");
let string = @import("std/string");

let main = fn(): i32 {
	let s = string.from("Hello World");
	defer s.deinit();
	let r = s.subRef(1, 6);

	io.println("In      : '", s, "'");
	io.println("- '--'  : ", s.find("--"));
	io.println("- 'H'   : ", s.find("H"));
	io.println("- ' '   : ", s.find(" "));
	io.println("- 'ld'  : ", s.find("ld"));

	io.println("\nIn      : '", r, "'");
	io.println("- '--'  : ", r.find("--"));
	io.println("- 'H'   : ", r.find("H"));
	io.println("- ' '   : ", r.find(" "));
	io.println("- 'W'   : ", r.find("W"));

	io.println("\nIn (rev): '", s, "'");
	io.println("- '--'  : ", s.rfind("--"));
	io.println("- 'H'   : ", s.rfind("H"));
	io.println("- ' '   : ", s.rfind(" "));
	io.println("- 'ld'  : ", s.rfind("ld"));

	io.println("\nIn (rev): '", r, "'");
	io.println("- '--'  : ", r.rfind("--"));
	io.println("- 'H'   : ", r.rfind("H"));
	io.println("- ' '   : ", r.rfind(" "));
	io.println("- 'W'   : ", r.rfind("W"));
	return 0;
};
