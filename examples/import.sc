let test = @import("./to_import");

let tmp = fn(): test.st {
	let s = test.st{5, 6, 7};
	s = test.st{1, 2, 3};
	return s;
};

let main = fn(): i32 {
	let q = test.x;
	let comptime res = test.add(q, q);
	let comptime p = tmp();
	return res + p.b; // should be 10 + 2
};