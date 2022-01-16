let c = @import("std/c");
let io = @import("std/io");

let main = fn(): i32 {
	for let i = 0; i < 10; ++i {
		let arr = c.malloc(i32, i + 1);
		defer c.free(i32, arr);
		// addr must be created because @as() cannot work with function calls that take reference
		// since @as() is a temporary but currently SimplifyPass cannot determine this to be a
		// function call (hence intermediate must be made manually)
		let addr = @as(u64, arr);
		io.println("Allocated address: ", addr);
		// c.free() is called here
	}
	return 0;
};
