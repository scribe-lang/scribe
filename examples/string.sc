let io = @import("std/io");
let string = @import("std/string");

let Point = struct {
	x: i32;
	y: u32;
	z: f64;
};

let str in const Point = fn(): string.String {
	let str = string.new();
	str.appendCStr("{");
	str.appendInt(self.x);
	str.appendCStr(", ");
	str.appendUInt(self.y);
	str.appendCStr(", ");
	str.appendFlt(self.z);
	str.appendCStr("}");
	return str;
};

let main = fn(): i32 {
	let point = Point{1, 2, 3.25};
	io.println("Point: ", point);
	// another way:
	let str = point.str();
	defer str.deinit();
	io.println("Point: ", str);
	return 0;
};