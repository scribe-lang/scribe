let c = @import("std/c");
let io = @import("std/io");

let comptime arr = @array(i32, 5, 5);
let comptime p = arr[2][1];
let anotherarr: @array(i8, 10); // this style can be used for struct fields and such

let main = fn(): i32 {
	let arr = @array(i32, 10, 32);
	for let i = 0; i < 10; ++i {
		for let j = 0; j < 32; ++j {
			arr[i][j] = i * j;
		}
	}
	c.strncpy(anotherarr, r"Hello", 6);
	io.println(anotherarr);
	for let i = 0; i < 10; ++i {
		for let j = 0; j < 32; ++j {
			io.println(arr[i][j]);
		}
	}
	return 0;
};