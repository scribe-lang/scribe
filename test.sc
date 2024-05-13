let Iter = struct<T, E> {
	of: &T;
	from: E;
	to: E;
	incr: E;
};

let comptime static x = 5;
// #inline
let p = inline fn(): i32 { return x * 200; };
#pack
let X = struct {
	#bytes=2
	a: u32;
	#bytes=2
	b: u32;
};

let X = union {
	a: u64;
	b: f64;
};

#vectorize,ompsimd=true
for let i = 0; i < 10; ++i { a[i] *= 10; }
inline if p == q { let n = X{1, 2}; }
else { let n = X{3, 4}; }

let f = 10;
// switch f { // switch works on numbers only
// case 5 {
// 	// ...
// }
// default {
// 	// ...
// }
// }

let s = "test".ref();

// let n = if s.isEmpty() {
// 	0
// } else {
// 	s.len()
// };

let func = fn(a: i32 = 25) {
	return a;
};

func();