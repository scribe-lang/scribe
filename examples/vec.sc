let c = @import("std/c");
let vec = @import("std/vec");

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();
	for let i = 0; i < 10; ++i {
		v.push(i);
	}
	for let i = 0; i < 5; ++i {
		v.pop();
	}
	let s = v.len().str();
	defer s.deinit();
	c.puts("Final length:");
	c.puts(s.cStr()); // should be 5
	return 0;
};