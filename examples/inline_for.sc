let f = fn(p: i32): i32 { // no need for 'comptime' before p, but the value actually has to be comptime
	let sum = 0;
	inline for let i = 1; i <= p; ++i {
		sum += i;
	}
	return sum;
};

let main = fn(): i32 {
	let n = 5;
	let comptime x = f(n); // should be 15
	return 0;
};