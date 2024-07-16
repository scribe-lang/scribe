let io = @import("std/io");

let Import = @typeOf(io);

let do = fn(x: Import) {
	x.println("hello world");
};

let main = fn(): i32 {
	// @compileError("err: ", @typeOf(io));
	return 0;
};