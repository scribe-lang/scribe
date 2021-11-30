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

inline if @isMainSrc() {

let main = fn(): i32 {
	let s = string.from("Hi there");
	defer s.deinit();
	print(s, "... On first line ", 1, ' ', 2);
	println(" continuing on first line ", 5, ' ', 10);
	return 0;
};

}