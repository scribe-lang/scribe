let io = @import("std/io");
let vec = @import("std/vec");
let string = @import("std/string");

let main = fn(): i32 {
	let v = vec.new(string.String, true);
	defer v.deinit();
	for let i = 0; i < 10; ++i {
		let s = string.from("Hi ");
		s.appendInt(i);
		v.push(s);
	}
	io.println("Vector: ", v);
	return 0;
};