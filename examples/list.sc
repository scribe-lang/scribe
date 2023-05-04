let io = @import("std/io");
let list = @import("std/list");

let main = fn(): i32 {
	let l = list.new(i32, true);
	defer l.deinit();
	for let i = 0; i < 10; ++i {
		l.push(i);
		io.println("count: ", l.len(), "; list: ", l);
	}
	for let i = 0; i < 5; ++i {
		l.pop();
		io.println("count: ", l.len(), "; list: ", l);
	}
	return 0;
};
