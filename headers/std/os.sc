let c = @import("std/c");
let fs = @import("std/fs");
let string = @import("std/string");

// this way is done so the value is usable at compile time - externs cannot be used at comptime
let comptime PATH_MAX = @sysPathMax();

// list of operating system names
let id = enum {
	Unknown,
	Linux,
	Windows,
	Apple,
	Android,
	FreeBSD,
	NetBSD,
	OpenBSD,
	DragonFly,
};

let comptime currentOS = @getOSID();

let getEnv = fn(key: *const i8): *const i8 {
	return c.getenv(key);
};
let setEnv = fn(key: *const i8, val: *const i8, overwrite: i1): i32 {
	return c.setenv(key, val, overwrite);
};

let system = c.system;

// allocates string
let getCWD = fn(): string.String {
	let path = string.withCap(PATH_MAX);
	c.getcwd(path.getBuf(), PATH_MAX);
	return path;
};
let setCWD = c.chdir;

let getExePath = fn(exe: *const i8): string.String {
	let res = string.new();
	if @as(u64, exe) == nil { return res; }
	let p = getEnv("PATH");
	if @as(u64, p) == nil { return res; }

	let path = string.from(p);
	defer path.deinit();
	let delimpath = path.delim(':');
	defer delimpath.deinit();

	for let i = 0; i < delimpath.len(); ++i {
		res = delimpath[i];
		res.appendCStr("/", 1);
		res.appendCStr(exe, 0);
		if fs.exists(res.cStr()) { break; }
		res.clear();
	}
	return res;
};