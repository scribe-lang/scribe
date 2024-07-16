/*
 * These are the memory management functions from C
 */

let _malloc = extern[malloc, "<stdlib.h>"] fn(size: u64): *void;
let _calloc = extern[calloc, "<stdlib.h>"] fn(count: u64, sz: u64): *void;
let _realloc = extern[realloc, "<stdlib.h>"] fn(data: *void, newsz: u64): *void;
let _free = extern[free, "<stdlib.h>"] fn(data: *void);
let _memcpy = extern[memcpy, "<string.h>"] fn(dest: *void, src: *const void, count: u64): *void;
let _memset = extern[memset, "<string.h>"] fn(dest: *void, ch: i32, count: u64): *void;
let _memcmp = extern[memcmp, "<string.h>"] fn(lhs: *const void, rhs: *const void, count: u64): i32;

let alloc = inline fn(comptime T: type, count: u64): *T {
        let comptime sz = @sizeOf(T);
        return @as(@ptr(T), _malloc(sz * count));
};
let calloc = inline fn(comptime T: type, count: u64): *T {
	let comptime sz = @sizeOf(T);
	return @as(@ptr(T), _calloc(count, sz));
};
let realloc = inline fn(comptime T: type, data: *T, count: u64): *T {
	let comptime sz = @sizeOf(T);
        return @as(@ptr(T), _realloc(@as(@ptr(void), data), sz * count));
};
let free = inline fn(comptime T: type, data: *T) {
        _free(@as(@ptr(void), data));
};
let cpy = inline fn(dest: any, src: const any, count: u64): any {
	return @as(@typeOf(dest), _memcpy(@as(@ptr(void), dest), @as(@ptr(void), src), count));
};
let set = inline fn(dest: any, ch: i32, count: u64): any {
	return @as(@typeOf(dest), _memset(@as(@ptr(void), dest), ch, count));
};
let cmp = inline fn(lhs: const any, rhs: const any, count: u64): i32 {
	return _memcmp(@as(@ptr(void), lhs), @as(@ptr(void), rhs), count);
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