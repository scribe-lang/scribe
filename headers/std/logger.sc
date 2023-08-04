let c = @import("std/c");
let io = @import("std/io");
let err = @import("std/err");
let vec = @import("std/vec");

let static current_level = 0;

let comptime LEVELCOUNT: const i32 = 5;

let Levels = enum {
	FATAL,
	WARN,
	INFO,
	DEBUG,
	TRACE
};

// red, yellow, magenta, cyan, green
let lvlColStr = fn(lvl: @enumTagTy(Levels)): *const i8 {
	if lvl == Levels.FATAL { return r"\\033[31m"; }
	if lvl == Levels.WARN { return r"\\033[33m"; }
	if lvl == Levels.INFO { return r"\\033[35m"; }
	if lvl == Levels.DEBUG { return r"\\033[36m"; }
	if lvl == Levels.TRACE { return r"\\033[32m"; }
	return r"";
};
let lvlStr = fn(lvl: @enumTagTy(Levels)): *const i8 {
	if lvl == Levels.FATAL { return r"FATAL"; }
	if lvl == Levels.WARN { return r"WARN"; }
	if lvl == Levels.INFO { return r"INFO"; }
	if lvl == Levels.DEBUG { return r"DEBUG"; }
	if lvl == Levels.TRACE { return r"TRACE"; }
	return r"INVALID";
};

let FileInfo = struct {
	f: *c.FILE;
	with_col: i1;
	must_close: i1;
};

let deinit in FileInfo = fn() {
	if self.must_close { self.f.close(); }
};

let targets: vec.Vec(FileInfo);

let setLevel = inline fn(lvl: i32) { current_level = lvl; };
let getLevel = inline fn(): i32 { return current_level; };

let addTarget = inline fn(target: *c.FILE, with_col: i1, must_close: i1) {
	targets.push(FileInfo{target, with_col, must_close});
};

let init = fn(use_stderr: i1) {
	targets.init(true);
	if use_stderr { addTarget(c.stderr, true, false); }
};

let deinit = inline fn() {
	targets.deinit();
};

let addTargetByName = fn(fname: *const i8, with_col: i1): i1 {
	if @as(u64, fname) == nil { return false; }
	let fp = c.fopen(fname, r"w+");
	if @as(u64, fp) == nil { return false; }
	addTarget(fp, with_col, true);
	return true;
};

let log = fn(lvl: i32, args: ...&const any) {
	if current_level < lvl { return; }
	let tim = c.time.time(nil);
	let timebuf = @array(i8, 30);
	timebuf[c.time.strftime(timebuf, @sizeOf(timebuf), r"%H:%M:%S", c.time.localtime(&tim))] = '\\0';

	for d in targets.each() {
		if d.with_col {
			io.fprintf(d.f, r"%s %s%+5s\\033[0m: ", timebuf, lvlColStr(lvl), lvlStr(lvl));
		} else {
			io.fprintf(d.f, r"%s %+5s: ", timebuf, lvlStr(lvl));
		}
		io.fprintln(d.f, args);
	}
};

let fatal = inline fn(args: ...&const any) { log(Levels.FATAL, args); };
let warn = inline fn(args: ...&const any) { log(Levels.WARN, args); };
let info = inline fn(args: ...&const any) { log(Levels.INFO, args); };
let debug = inline fn(args: ...&const any) { log(Levels.DEBUG, args); };
let trace = inline fn(args: ...&const any) { log(Levels.TRACE, args); };