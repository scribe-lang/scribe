let c = @import("std/c");
let fs = @import("std/fs");
let core = @import("std/core");
let string = @import("std/string");

let system = c.system;

// this way is done so the value is usable at compile time - externs cannot be used at comptime
let comptime PATH_MAX = @sysPathMax();

// these exist here for ease of use and being intuitive
// any they exist in std/core so they're usable in any
// source (std/core has no dependency)
let id = core.os;
let comptime currentOS = core.currentOS;

let setCWD = c.unistd.chdir;

let getEnv = fn(key: *const i8): *const i8 {
	return c.getenv(key);
};
let setEnv = fn(key: *const i8, val: *const i8, overwrite: i1): i32 {
	return c.setenv(key, val, overwrite);
};

let dirName = fn(path: *const i8): string.String {
	let p = string.withCap(PATH_MAX);
	defer p.deinit();
	p.append(path);
	let res = c.basename(p.getBuf());
	return string.from(res);
};

let baseName = fn(path: *const i8): string.String {
	let p = string.withCap(PATH_MAX);
	defer p.deinit();
	p.append(path);
	let res = c.dirname(p.getBuf());
	return string.from(res);
};

// allocates string
let getCWD = fn(): string.String {
	let path = string.withCap(PATH_MAX);
	c.unistd.getcwd(path.getBuf(), PATH_MAX);
	return path;
};

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
		res.append(exe);
		if fs.exists(res.cStr()) { break; }
		res.clear();
	}
	return res;
};