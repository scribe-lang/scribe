let io = @import("std/io");
let vec = @import("std/vec");
let string = @import("std/string");

let accum = fn(data: &const i32, sum: &i32) {
	sum += data;
};

let doEach in vec.Vec = fn(cb: any, args: ...&any) {
	for let i = 0; i < self.length; ++i {
		cb(self.data[i], args);
	}
};

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();
	for let i = 0; i < 10; ++i {
		v.push(i);
	}
	let sum = 0;
	v.doEach(accum, sum);
	io.println("Sum: ", sum);
	return 0;
};