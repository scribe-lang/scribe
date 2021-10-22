let global i32 = @createIntType(32, true);
let global i64 = @createIntType(64, true);

let x = 5;

let add = fn(a: i32, b: i32): i32 {
	return a + b;
};