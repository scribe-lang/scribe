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

let getEnv = fn(key: *const i8): string.String {
	let val = c.getenv(key);
	let res = string.new();
	if @as(u64, val) == nil { return res; }
	res.append(val);
	return res;
};
let setEnv = fn(key: *const i8, val: *const i8, overwrite: i1): i32 {
	return c.setenv(key, val, overwrite);
};

let dirName = fn(path: *const i8): string.String {
	let p: @array(i8, PATH_MAX);
	c.strcpy(p, path);
	c.dirname(p);
	return string.from(p);
};

let baseName = fn(path: *const i8): string.String {
	let p: @array(i8, PATH_MAX);
	c.strcpy(p, path);
	c.basename(p);
	return string.from(p);
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
	let path = getEnv("PATH");
	defer path.deinit();
	if path.isEmpty() { return path; }

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