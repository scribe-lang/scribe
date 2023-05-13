let io = @import("std/io");
let os = @import("std/os");
let vec = @import("std/vec");
let thread = @import("std/thread");

let mtrun: os.MultiThreadedExec;

let func = fn(data: *void): *void {
	let tid = *@as(@ptr(i32), data);
	mtrun.exec("echo 'current thread: ", thread.getSelfId(), " with data: ", tid, "'");
	return nil;
};

let main = fn(): i32 {
	mtrun.init();
	defer mtrun.deinit();
	let v = vec.new(thread.Thread, true);
	defer v.deinit();
	for let i = 0; i < 10; ++i {
		v.push(thread.new());
		v.back().run(func, @as(@ptr(void), &i));
	}
	for e in v.each() {
		e.join();
	}
	return 0;
};