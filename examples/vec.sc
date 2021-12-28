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
	// subscript function
	io.println("Changing item ", v[1], " at index 1 to 25");
	v[1] = 25;
	let s = v.str();
	defer s.deinit();
	io.println("Final Vector: ", s);
	return 0;
};