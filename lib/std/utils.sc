/*
 * Contains utility functions for... well, everything
 */

let c = @import("std/c");

let countIntDigits = fn(data: i64): i32 {
	let res = @as(i32, !data);
	// for minus sign
	if data < 0 {
		++res;
		if data == c.i64min {
			data = c.i64max;
		} else {
			data = -data;
		}
	}
	while data > 0 {
		data /= 10;
		++res;
	}
	return res;
};

let countUIntDigits = fn(data: u64): i32 {
	let res = @as(i32, !data);
	while data {
		++res;
		data /= 10;
	}
	return res;
};
