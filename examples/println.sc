let puts = extern[puts, "<stdio.h>"] fn(data: *const u8): i32;

let strlen = fn(data: *const i8): i32 {
	let ptr = data;
	let i = 0;
	while ptr[i++] != 0 {}
	return i - 1;
};

let println = fn(comptime fmt: *const i8, va: ...any): i32 {
	let len = strlen(fmt);
	let j = 0;
	inline for let i = 0; i < len; ++i {
		// ...
	}
	let res = puts(fmt);
	res += puts("\n");
	return res;
};