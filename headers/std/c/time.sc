let ctype = @import("std/c/types");

let comptime time_t = ctype.long;

let tm = extern[struct tm, "<time.h>"] struct {
	tm_sec: i32;
	tm_min: i32;
	tm_hour: i32;
	tm_mday: i32;
	tm_mon: i32;
	tm_year: i32;
	tm_wday: i32;
	tm_yday: i32;
	tm_isdst: i32;
	tm_gmtoff: ctype.long;
	tm_zone: *const i8;
};

let time = extern[time, "<time.h>"] fn(timer: *const time_t): time_t;
let localtime = extern[localtime, "<time.h>"] fn(timer: *const time_t): *tm;
let strftime = extern[strftime, "<time.h>"] fn(str: *i8, count: u64, format: *const i8, time: *const tm): u64;
