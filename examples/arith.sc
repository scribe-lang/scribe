let global i32 = @createIntType(32, true);
let global i64 = @createIntType(64, true);

let calc = fn(a: i32, b: i32): i32 {
	let res = a + b;
	res += a / b;
	res -= a % b;
	res /= 2;
	res %= 2000;
	res ^= 200;
	return res;
};

let comptime res = calc(5, 10);