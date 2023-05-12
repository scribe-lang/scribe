let io = @import("std/io");
let list = @import("std/list");

let accum = fn(data: &const i32, sum: &i32) { sum += data; };

// list.List has a doEach method with signature: List.doEach(callback: fn(data: &const List.E, args: ...&any), args: ...&any)

let main = fn(): i32 {
	let l = list.new(i32, true);
	defer l.deinit();
	for let i = 0; i < 10; ++i {
		l.push(i);
		io.println("count: ", l.len(), "; list: ", l);
	}
	let sum = 0;
	l.doEach(accum, sum);
	io.println("sum of all elements: ", sum);
	for let i = 0; i < 5; ++i {
		l.pop();
		io.println("count: ", l.len(), "; list: ", l);
	}
	return 0;
};
