let c = @import("std/c");
let string = @import("std/string");

let stdin = c.stdin;
let stdout = c.stdout;
let stderr = c.stderr;

let fprint = fn(f: *c.FILE, data: ...&const any): i32 {
	let comptime len = @valen();
	let sum = 0;
	inline for let comptime i = 0; i < len; ++i {
		inline if @isEqualTy(data[i], string.String) {
			sum += c.fputs(data[i].cStr(), f);
		} elif @isEqualTy(data[i], string.StringRef) {
			sum += fprintf(f, "%.*s", data[i].len(), data[i].data());
		} elif @isFlt(data[i]) {
			sum += c.fprintf(f, c.getTypeSpecifier(@typeOf(data[i])), string.getPrecision(), data[i]);
		} elif @isPrimitiveOrPtr(data[i]) {
			sum += c.fprintf(f, c.getTypeSpecifier(@typeOf(data[i])), data[i]);
		} else {
			let s = data[i].str();
			defer s.deinit();
			sum += c.fputs(s.cStr(), f);
		}
	}
	return sum;
};

let fprintf = fn(f: *c.FILE, fmt: *const i8, data: ...&const any): i32 {
	let comptime len = @valen();
	inline for let comptime i = 0; i < len; ++i {
		inline if !@isPrimitiveOrPtr(data[i]) && !@isCString(data[i]) {
			@compileError("Only primitive types and C strings can be passed to fprintf, found: ", @typeOf(data[i]));
		}
	}
	return c.fprintf(f, fmt, data);
};

let fprintln = fn(f: *c.FILE, data: ...&const any): i32 {
	return fprint(f, data, '\n');
};
let print = fn(data: ...&const any): i32 {
	return fprint(c.stdout, data);
};
let println = fn(data: ...&const any): i32 {
	return print(data, '\n');
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Filesystem Functions
///////////////////////////////////////////////////////////////////////////////////////////////////

let fopen = c.fopen;
let close in c.FILE = fn(): i32 { return c.fclose(&self); };
let flush in c.FILE = fn(): i32 { return c.fflush(&self); };
let print in c.FILE = fn(data: ...&const any): i32 {
	return fprint(&self, data);
};
let println in c.FILE = fn(data: ...&const any): i32 {
	return fprintln(&self, data);
};
let printf in c.FILE = fn(fmt: *const i8, data: ...&const any): i32 {
	return fprintf(&self, fmt, data);
};
let read in c.FILE = fn(buf: &string.String): i64 {
	buf.deinit();
	let bufsz: u64 = 0;
	let sz = c.getline(&buf.getBuf(), &bufsz, &self);
	if sz > 0 {
		buf.length = sz;
		buf.capacity = buf.length + 1;
	}
	return sz;
};

inline if @isMainSrc() {

let main = fn(): i32 {
	let s = string.from("Hi there");
	defer s.deinit();
	print(s, "... On first line ", 1, ' ', 2);
	println(" continuing on first line ", 5, ' ', 10);
	return 0;
};

}