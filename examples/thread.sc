let io = @import("std/io");
let thread = @import("std/thread");

let func = fn(data: *void): *void {
	let tid = @as(@ptr(u64), data);
	io.println("Hello world from thread ", thread.getSelf(), " with data: ", *tid);
	return nil;
};

let main = fn(): i32 {
	let data: u64 = 5;
	let t = thread.new();
	let t2 = thread.new();
	t.run(func, @as(@ptr(void), &data));
	t2.run(func, @as(@ptr(void), &data));
	t.join();
	t.run(func, @as(@ptr(void), &data));
	t.join();
	return 0;
};