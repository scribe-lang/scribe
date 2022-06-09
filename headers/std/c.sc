/*
 * These are the core C functions/libraries used across standard library
 */

let grp = @import("std/c/grp");
let pwd = @import("std/c/pwd");
let stat = @import("std/c/stat");
let time = @import("std/c/time");
let ioctl = @import("std/c/ioctl");
let dirent = @import("std/c/dirent");
let unistd = @import("std/c/unistd");

let FILE = extern[FILE, "<stdio.h>"] struct {};

let stdin: *FILE = extern[stdin, "<stdio.h>"];
let stdout: *FILE = extern[stdout, "<stdio.h>"];
let stderr: *FILE = extern[stderr, "<stdio.h>"];

let errno: i32 = extern[errno, "<errno.h>"];

let popen = extern[popen, "<stdio.h>"] fn(cmd: *const i8, mode: *const i8): *FILE;
let pclose = extern[pclose, "<stdio.h>"] fn(file: *FILE): i32;
let fopen = extern[fopen, "<stdio.h>"] fn(name: *const i8, mode: *const i8): *FILE;
let fclose = extern[fclose, "<stdio.h>"] fn(file: *FILE): i32;
let fflush = extern[fflush, "<stdio.h>"] fn(file: *FILE): i32;
let fputs = extern[fputs, "<stdio.h>"] fn(data: *const i8, file: *FILE): i32;
let fprintf = extern[fprintf, "<stdio.h>"] fn(file: *FILE, fmt: *const i8, args: ...any): i32;
let getline = extern[getline, "<stdio.h>"] fn(buf: **i8, size: *u64, file: *FILE): i64;
let putc = extern[putc, "<stdio.h>"] fn(data: const i8, file: *FILE): i32;
let fputc = extern[fputc, "<stdio.h>"] fn(data: const i8, file: *FILE): i32;
let puts = extern[puts, "<stdio.h>"] fn(data: *const i8): i32;
let snprintf = extern[snprintf, "<stdio.h>"] fn(buf: *i8, bufsz: u64, fmt: *const i8, args: ...any): i32;
let strlen = extern[strlen, "<string.h>"] fn(data: *const i8): u64;
let strcmp = extern[strcmp, "<string.h>"] fn(lhs: *const i8, rhs: *const i8): i32;
let strcpy = extern[strcpy, "<string.h>"] fn(dest: *i8, src: *const i8): *i8;
let strncmp = extern[strncmp, "<string.h>"] fn(lhs: *const i8, rhs: *const i8, count: u64): i32;
let strncpy = extern[strncpy, "<string.h>"] fn(dest: *i8, src: *const i8, count: u64): *i8;
let getenv = extern[getenv, "<stdlib.h>"] fn(name: *const i8): *const i8; // in C, actually returns *i8
let setenv = extern[setenv, "<stdlib.h>"] fn(name: *const i8, val: *const i8, overwrite: i1): i32;
let system = extern[system, "<stdlib.h>"] fn(command: *const i8): i32;
let isspace = extern[isspace, "<ctype.h>"] fn(ch: i32): i32;
let strerror = extern[strerror, "<string.h>"] fn(errnum: i32): *const i8;
let dirname = extern[dirname, "<libgen.h>"] fn(path: *i8): *i8;
let basename = extern[basename, "<libgen.h>"] fn(path: *i8): *i8;
let _wexitstatus = extern[WEXITSTATUS, "<stdlib.h>"] fn(status: i32): i32;

let getTypeSpecifier = inline fn(comptime ty: type): *const i8 {
	inline if @isEqualTy(ty, i1) || @isEqualTy(ty, i16) || @isEqualTy(ty, i32) {
		return "%d";
	} elif @isEqualTy(ty, u8) || @isEqualTy(ty, u16) || @isEqualTy(ty, u32) {
		return "%u";
	} elif @isEqualTy(ty, i64) {
		return "%lld";
	} elif @isEqualTy(ty, u64) {
		return "%llu";
	} elif @isEqualTy(ty, i8) {
		return "%c";
	} elif @isEqualTy(ty, f32) {
		return "%.*f";
	} elif @isEqualTy(ty, f64) {
		return "%.*lf";
	} elif @isCString(ty) {
		return "%s";
	} elif @isPtr(ty) {
		return "%p";
	}
	return nil;
};

let wexitstatus = inline fn(status: i32): i32 {
	return _wexitstatus(status);
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Numeric Limits
///////////////////////////////////////////////////////////////////////////////////////////////////

let comptime i32min: const i32 = -2147483648;
let comptime i32max: const i32 = 2147483647;
let comptime i64min: const i64 = -9223372036854775807;
let comptime i64max: const i64 = 9223372036854775807;
let comptime u64max: const u64 = 18446744073709551615;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Tests
///////////////////////////////////////////////////////////////////////////////////////////////////

inline if @isMainSrc() {

let main = fn(): i32 {
	return 0;
};

}