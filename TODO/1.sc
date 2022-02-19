let tmp = fn(data: &i32) {
	data += 10;
};

let X = fn(d: i32): i32 {
	let d = d; // should be invalid (already existing variable)
	return d;
};

let main = fn(): i32 {
	tmp(5); // should be invalid (cannot use const as parameter for reference)
	return 0;
};
