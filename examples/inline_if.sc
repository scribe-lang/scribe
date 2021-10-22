let global i32 = @createIntType(32, true);
let global i64 = @createIntType(64, true);

let i = 10;

inline if 0 {
	let p = 20;
} else {
	let q = 100;
}