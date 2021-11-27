let c = @import("std/c");
let io = @import("std/io");
let vec = @import("std/vec");

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();
	for let i = 0; i < 10; ++i {
		v.push(i);
	}
	for let i = 0; i < 5; ++i {
		v.pop();
	}
	io.println("Final length: ", v.len());
	return 0;
};