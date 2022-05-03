let io = @import("std/io");
let vec = @import("std/vec");
let sorting = @import("std/sorting");

let main = fn(): i32 {
	let v = vec.new(*i8, true);
	defer v.deinit();

	v.push("Hello");
	v.push("Hi");
	v.push("There");
	v.push("I");
	v.push("Go");

	io.println("Original: ", v);
	v.sort(sorting.cStrCmp);
	io.println("Sorted:   ", v);
	return 0;
};