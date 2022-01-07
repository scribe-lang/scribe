let io = @import("std/io");
let map = @import("std/map");
let string = @import("std/string");

let main = fn(): i32 {
	let dict1 = map.new(@ptr(i8), i32);
	defer dict1.deinit();
	let k = "ABC";
	let v = 10;
	dict1.add(k, v);
	if dict1.find(k) {
		io.println("found ", k, " = ", dict1.get(k));
	}

	let dict2 = map.new(string.String, string.String);
	defer dict2.deinit();
	let k2 = string.from("ABC");
	let v2 = string.from("XYZ");
	// k2, v2 deinit by dict2

	dict2.add(k2, v2);
	if dict2.find(k2) {
		io.println("found ", k2, " = ", dict2.get(k2));
	}
	if !dict2.find(v2) {
		io.println("not found ", v2);
	}
	return 0;
};