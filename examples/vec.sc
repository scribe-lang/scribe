let io = @import("std/io");
let vec = @import("std/vec");

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();
	v.reserve(10);
	for let i = 0; i < 10; ++i {
		v.push(i);
	}
	for let i = 0; i < 5; ++i {
		v.pop();
	}
	// subscript function
	io.println("Changing item ", v[1], " at index 1 to 25");
	v[1] = 25;
	io.println("Final Vector: ", v); // calls v.str() internally
	return 0;
};