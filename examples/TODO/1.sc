let X = fn(d: i32): i32 {
	let d = d; // should be invalid (already existing variable)
	return d;
};

let main = fn(): i32 {
	X(5);
	return 0;
};
