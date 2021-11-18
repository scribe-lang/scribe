let add = fn(data: ...i32): i32 {
	let comptime len = @valen();
	let res = 0;
	inline for let comptime i = 0; i < len; ++i {
		res += data[i];
	}
	return res + len;
};

let comptime res = add(5, 6, 7, 8); // must be 30