let io = @import("std/io");
let vec = @import("std/vec");
let string = @import("std/string");

let accum = fn(data: &const i32, sum: &i32, mul: &i32) { sum += data; mul *= data; };

// vec.Vec has a doEach method with signature: Vec.doEach(callback: fn(data: &const Vec.T, args: ...&any), args: ...&any)

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();
	for let i = 1; i <= 10; ++i {
		v.push(i);
	}
	let sum = 0;
	let mul = 1;
	v.doEach(accum, sum, mul);
	io.println("Sum: ", sum); // Sum: 55
	io.println("Mul: ", mul); // Mul: 3628800
	return 0;
};