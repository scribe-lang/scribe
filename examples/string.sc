let io = @import("std/io");
let string = @import("std/string");

let Point = struct {
	x: i32;
	y: u32;
	z: f64;
};

let str in const Point = fn(): string.String {
	let str = string.new();
	str.appendCStr("{", 1);
	str.appendInt(self.x);
	str.appendCStr(", ", 2);
	str.appendUInt(self.y);
	str.appendCStr(", ", 2);
	str.appendFlt(self.z);
	str.appendCStr("}", 1);
	return str;
};

let main = fn(): i32 {
	let point = Point{1, 2, 3.25};
	io.println("Point: ", point);
	// another way:
	let str = point.str();
	defer str.deinit();
	io.println("Point: ", str);

	// subscript test
	let a = string.from("Hello");
	defer a.deinit();
	a[4] = 'a';
	io.println(a); // "Hella"

	// sub C-string test
	let b = string.fromSubCStr("Hello", 4);
	defer b.deinit();
	io.println(b);

	// StringRef test
	let r1 = b.subRef(1, 2);
	let r2 = b.subRef(1, 1);
	let r3 = b.subRef(1, 2);
	io.println(r1);
	io.println(r2);
	io.println(r1 == r3); // should be true
	io.println(r1 == r2); // should be false
	return 0;
};