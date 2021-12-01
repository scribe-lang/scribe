/*
 * These are the core C functions used across standard library
 */

let FILE = extern[FILE, "<stdio.h>"] struct {};

let stdin: *FILE = extern[stdin, "<stdio.h>"];
let stdout: *FILE = extern[stdout, "<stdio.h>"];
let stderr: *FILE = extern[stderr, "<stdio.h>"];

let fopen = extern[fopen, "<stdio.h>"] fn(name: *const i8, mode: *const i8): *FILE;
let fclose = extern[fclose, "<stdio.h>"] fn(file: *FILE): i32;
let fflush = extern[fflush, "<stdio.h>"] fn(file: *FILE): i32;
let putc = extern[putc, "<stdio.h>"] fn(data: const i8): i32;
let fputc = extern[fputc, "<stdio.h>"] fn(data: const i8, file: *FILE): i32;
let puts = extern[puts, "<stdio.h>"] fn(data: *const i8): i32;
let fputs = extern[fputs, "<stdio.h>"] fn(data: *const i8, file: *FILE): i32;
let fprintf = extern[fprintf, "<stdio.h>"] fn(file: *FILE, fmt: *const i8, args: ...any): i32;
let snprintf = extern[snprintf, "<stdio.h>"] fn(buf: *i8, bufsz: u64, fmt: *const i8, args: ...any): i32;
let strlen = extern[strlen, "<string.h>"] fn(data: *const i8): u64;
let strcmp = extern[strcmp, "<string.h>"] fn(lhs: *const i8, rhs: *const i8): i32;
let _malloc = extern[malloc, "<stdlib.h>"] fn(size: u64): *void;
let _realloc = extern[realloc, "<stdlib.h>"] fn(data: *void, newsz: u64): *void;
let _free = extern[free, "<stdlib.h>"] fn(data: *void);
let memcpy = extern[memcpy, "<string.h>"] fn(dest: *void, src: *const void, count: u64): *void;
let memset = extern[memset, "<string.h>"] fn(dest: *void, ch: i32, count: u64): *void;
let getenv = extern[getenv, "<stdlib.h>"] fn(name: *const i8): *const i8; // in C, actually returns *i8
let setenv = extern[setenv, "<stdlib.h>"] fn(name: *const i8, val: *const i8, overwrite: i1): i32;

let malloc = fn(comptime T: type, count: u64): *T {
        let comptime sz = @sizeOf(T);
        return @as(@ptr(T), _malloc(sz * count));
};
let realloc = fn(comptime T: type, data: *T, count: u64): *T {
	let comptime sz = @sizeOf(T);
        return @as(@ptr(T), _realloc(@as(@ptr(void), data), sz * count));
};
let free = fn(comptime T: type, data: *T) {
        _free(@as(@ptr(void), data));
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