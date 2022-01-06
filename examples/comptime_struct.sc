let st = struct {
	x: i32;
	y: i32;
};

let getX in st = fn(): i32 {
	return self.x;
};

let getY in st = fn(): i32 {
	return self.y;
};

let some = fn(idx: i32, x_or_y: i1): i32 {
	let a = @array(st, 10);
	for let i = 0; i < 10; ++i {
		a[i].x = i * 2 + 1;
		a[i].y = i * 3 + 2;
	}
	if x_or_y {
		return a[idx].getX();
	}
	return a[idx].getY();
};

let main = fn(): i32 {
	let comptime p = some(2, true) + some(3, false); // p = 5 + 11 = 16
	return p;
};
