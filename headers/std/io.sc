let c = @import("std/c");
let string = @import("std/string");

let fprint = fn(f: *c.FILE, data: ...&const any): i32 {
	let comptime len = @valen();
	let sum = 0;
	inline for let comptime i = 0; i < len; ++i {
		inline if @isCString(data[i]) {
			sum += c.fputs(data[i], f);
		} elif @isCChar(data[i]) {
			sum += c.fputc(data[i], f);
		} elif @isEqualTy(data[i], string.String) {
			sum += c.fputs(data[i].cStr(), f);
		} else {
			let s = data[i].str();
			defer s.deinit();
			sum += c.fputs(s.cStr(), f);
		}
	}
	return sum;
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
let read in c.FILE = fn(buf: &string.String): i64 {
	buf.deinit();
	let res = c.getline(&buf.getBuf(), &buf.length, &self);
	if buf.length > 0 {
		buf.capacity = buf.length + 1;
	}
	return res;
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