let io = @import("std/io");
let mutex = @import("std/mutex");
let thread = @import("std/thread");

let mtx: mutex.Mutex;

let func = fn(data: *void): *void {
	let tid = @as(@ptr(u64), data);
	mtx.lock();
	io.println("Hello world from thread ", thread.getSelfId(), " with data: ", *tid);
	mtx.unlock();
	return nil;
};

let main = fn(): i32 {
	mtx.init();
	defer mtx.deinit();

	io.println("Hardware concurrency: ", thread.getConcurrency());

	let data: u64 = 5;
	let t = thread.new();
	let t2 = thread.new();

	t.run(func, @as(@ptr(void), &data));
	t2.run(func, @as(@ptr(void), &data));
	t.join();
	t.run(func, @as(@ptr(void), &data));
	t.join();
	t2.join();
	return 0;
};