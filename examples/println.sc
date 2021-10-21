let puts = extern[puts, "<stdio.h>"] fn(data: *const u8): i32;

let strlen = fn(data: *const i8): i32 {
	let ptr = data;
	let i = 0;
	while ptr[i++] != 0 {}
	return i - 1;
};

// a variadic function is guaranteed to be specialized
let println = fn(args: ...any): i32 {
	let comptime len = @len(args);
	let res = 0;
	inline for let comptime i = 0; i < len; ++i {
		inline if @baseTypeName(args[i]) == "i1" {
			res += printI1(args[i]);
		} elif @isTypeIntegral(args[i]) == "u8" {
			res += printInt(args[i]);
		} elif @isTypeFloat(args[i]) {
			res += printFlt(args[i]);
		} elif @baseTypeName(args[i]) == "*i8" { // does not care about const, static, ...
			res += puts(args[i]);
		} elif @isPtr(args[i]) && if @isPrimitive(*args[i]) {
			res += puts(@fullTypeName(args[i]));
		} else {
			let str = args[i].toStr();
			defer str.deinit();
			res += puts(str.getCStr());
		}
	}
	res += puts("\n");
	return res;
};