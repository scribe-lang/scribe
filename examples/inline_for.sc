let f = fn(p: i32): i32 {
	let sum = 0;
	inline for let i = 1; i <= p; ++i {
		sum += i;
	}
	return sum;
};

let n = 5;
let comptime x = f(n);