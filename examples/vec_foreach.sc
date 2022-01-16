let io = @import("std/io");
let vec = @import("std/vec");

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();
	for let i = 0; i < 10; ++i {
		v.push(i);
	}
	for e in v.each() {
		io.println(e);
	}
	for e in v.eachRev() {
		io.println(e);
	}
	for e in v.each() {
		e = e + 1;
	}
	io.println(v);
	return 0;
};
