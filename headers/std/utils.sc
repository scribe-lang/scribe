/*
 * Contains utility functions for... well, everything
 */

let c = @import("std/c");

let countIntDigits = fn(data: i64): i32 {
	let res = 0;
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
	if res == 0 { res = 1; }
	return res;
};

let countUIntDigits = fn(data: u64): i32 {
	let res = 0;
	while data > 0 {
		data /= 10;
		++res;
	}
	if res == 0 { res = 1; }
	return res;
};