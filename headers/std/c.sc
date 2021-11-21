let puts = extern[puts, "<stdio.h>"] fn(data: *const i8): i32;
let snprintf = extern[snprintf, "<stdio.h>"] fn(buf: *i8, bufsz: u64, fmt: *const i8, args: ...any): i32;
let strlen = extern[strlen, "<string.h>"] fn(data: *const i8): i32;
let _malloc = extern[malloc, "<stdlib.h>"] fn(size: u64): *void;
let _realloc = extern[realloc, "<stdlib.h>"] fn(data: *void, newsz: u64): *void;
let _free = extern[free, "<stdlib.h>"] fn(data: *void);
let _memcpy = extern[memcpy, "<string.h>"] fn(dest: *void, src: *const void, count: u64): *void;
let _memset = extern[memset, "<string.h>"] fn(dest: *void, ch: i32, count: u64): *void;

let malloc = fn(comptime T: type, count: u64): *T {
        let comptime sz = @sizeOf(T);
        return @as(@ptr(T), _malloc(sz * count));
};
let realloc = fn(comptime T: type, data: *T, count: u64): *T {
        return @as(@ptr(T), _realloc(@as(@ptr(void), data), count));
};
let free = fn(comptime T: type, data: *T) {
        _free(@as(@ptr(void), data));
};

let memcpy = fn(dest: *void, src: *const void, count: u64) {
        _memcpy(dest, src, count);
};
let memset = fn(dest: *void, fill_byte: i32, count: u64) {
        _memset(dest, fill_byte, count);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Numeric Limits
///////////////////////////////////////////////////////////////////////////////////////////////////

let i32Min = fn(): i32 {
        return -2147483648;
};

let i32Max = fn(): i32 {
        return 2147483647;
};

let i64Min = fn(): i64 {
        return -9223372036854775807;
};

let i64Max = fn(): i64 {
        return 9223372036854775807;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

inline if @isMainSrc() {

let main = fn(): i32 {
	let v = malloc(i32, 20);
	defer free(i32, v);
	for let i = 0; i < 20; ++i {
		v[i] = i;
	}
	let v2 = malloc(i64, 20);
	defer free(i64, v2);
	for let i = 0; i < 20; ++i {
		v2[i] = i;
	}
	return 0;
};

}