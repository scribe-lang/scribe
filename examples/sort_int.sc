let io = @import("std/io");
let vec = @import("std/vec");
let sorting = @import("std/sorting");

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();

	v.push(5);
	v.push(1);
	v.push(2);
	v.push(7);
	v.push(6);
	v.push(3);
	v.push(4);
	v.push(8);
	v.push(10);
	v.push(9);

	io.println("Original: ", v);
	v.sort(sorting.i32Cmp);
	io.println("Sorted:   ", v);
	return 0;
};