let io = @import("std/io");

let comptime arr = @array(i32, 5, 5);
let comptime p = arr[2][1];

let main = fn(): i32 {
	let arr = @array(i32, 10, 32);
	for let i = 0; i < 10; ++i {
		for let j = 0; j < 32; ++j {
			arr[i][j] = i * j;
		}
	}
	for let i = 0; i < 10; ++i {
		for let j = 0; j < 32; ++j {
			io.println(arr[i][j]);
		}
	}
	return 0;
};