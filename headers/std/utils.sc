/*
 * Contains utility functions for... well, everything
 */

let c = @import("std/c");

let countDigits = fn(data: i64): i32 {
	let res = 0;
	// for minus sign
	if data < 0 {
		++res;
		if data == c.i64Min() {
			data = c.i64Max();
		} else {
			data = -data;
		}
	}
	while data > 9 {
		data /= 10;
		++res;
	}
	return res;
};