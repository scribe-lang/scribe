let io = @import("std/io");
let vec = @import("std/vec");

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();
	for let i = 0; i < 20; ++i {
		v.push(i);
	}
	io.println("vector contents:");
	io.println(v);
	v.pop();
	return 0;
};