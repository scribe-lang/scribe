let c = @import("std/c");
let string = @import("std/string");

let print = fn(data: ...any): i32 {
	let comptime len = @valen();
	let sum = 0;
	inline for let comptime i = 0; i < len; ++i {
		inline if @isCString(data[i]) {
			sum += c.fputs(data[i], c.stdout);
		} elif @isCChar(data[i]) {
			sum += c.fputc(data[i], c.stdout);
		} else {
			let s = data[i].str();
			defer s.deinit();
			sum += c.fputs(s.cStr(), c.stdout);
		}
	}
	return sum;
};

let println = fn(data: ...any): i32 {
	let comptime len = @valen();
	let sum = 0;
	inline for let comptime i = 0; i < len; ++i {
		inline if @isCString(data[i]) {
			sum += c.fputs(data[i], c.stdout);
		} elif @isCChar(data[i]) {
			sum += c.fputc(data[i], c.stdout);
		} else {
			let s = data[i].str();
			defer s.deinit();
			sum += c.fputs(s.cStr(), c.stdout);
		}
	}
	sum += c.fputc('\n', c.stdout);
	return sum;
};

inline if @isMainSrc() {

let main = fn(): i32 {
	print("On first line ", 1, ' ', 2);
	println(" continuing on first line ", 5, ' ', 10);
	return 0;
};

}