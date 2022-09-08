let sum = fn(args: ...any): i32 {
	let comptime len = @valen();
	let sum: i64 = 0;
	inline for let comptime i = 0; i < len; ++i {
		inline if !@isEqualTy(args[i], i32) && !@isEqualTy(args[i], i64) {
			@compileError("Expected argument type to be either i32 or i64, found: ", @typeOf(args[i]));
		}
		sum += args[i];
	}
	return sum;
};

let main = fn(): i32 {
	let s1 = sum(1);
	let comptime s2 = sum(1, 2, 3, 4.5); // compilation fails - 4.5 is neither i32 nor i64
	return 0;
};
