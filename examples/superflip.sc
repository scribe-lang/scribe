let c = @import("std/c");
let io = @import("std/io");

let superFlip = fn(input: i1, started: &i1, ctr: &i32): i1 {
	if !started {
		if input { started = true; }
		return input;
	}

	if input { --ctr; return true; }
	return ctr > 0;
};

let attempt = fn(data: *const i8) {
	let len = c.strlen(data);
	let started = false;
	let ctr = 2;
	io.print("input:  ", data, "\noutput: ");
	for let i = 0; i < len; ++i {
		if superFlip(data[i] == '1', started, ctr) {
			io.print("1");
		} else {
			io.print("0");
		}
	}
	io.println();
};

let main = fn(): i32 {
	attempt("0001000000010010000"); // 0001111111111110000
	attempt("0000010000000110000"); // 0000011111111110000
	attempt("0100000000000010010"); // 0111111111111111110
	return 0;
};
