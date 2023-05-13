let unistd = @import("std/c/unistd");

let pthread_t: u64 = extern[pthread_t, "<pthread.h>", "-pthread"];
let pthread_attr_t = extern[pthread_attr_t, "<pthread.h>", "-pthread"] struct {};

let pthread_create = extern[pthread_create, "<pthread.h>", "-pthread"] fn(id: *pthread_t, attr: *const pthread_attr_t, start_routine: fn(data: *void): *void, arg: *void): i32;
let pthread_join = extern[pthread_join, "<pthread.h>", "-pthread"] fn(id: pthread_t, value_ptr: **void): i32;

let getSelfId = extern[pthread_self, "<pthread.h>", "-pthread"] fn(): pthread_t;

let getConcurrency = inline fn(): i64 {
	return unistd.sysconf(unistd._SC_NPROCESSORS_ONLN);
};

let Thread = struct {
	threadid: pthread_t;
};

let new = inline fn(): Thread {
	return Thread{0};
};
let deinit in Thread = inline fn() {};

let getId in Thread = inline fn(): pthread_t {
	return self.threadid;
};

let run in Thread = inline fn(routine: fn(data: *void): *void, arg: *void): i32 {
	return pthread_create(&self.threadid, nil, routine, arg);
};

let join in Thread = inline fn(): i32 {
	return pthread_join(self.threadid, nil);
};

let getCurrent = inline fn(): Thread {
	return Thread{getSelfId()};
};

// usage
inline if @isMainSrc() {

let io = @import("std/io");

let func = fn(data: *void): *void {
	let tid = @as(u64, data);
	io.println("Hello world from thread ", getSelfId(), " with data: ", tid);
	return nil;
};

let main = fn(): i32 {
	let data: u64 = 5;
	let t = new();
	let t2 = new();
	t.run(func, @as(@ptr(void), data));
	t2.run(func, @as(@ptr(void), data));
	t.join();
	t.run(func, @as(@ptr(void), data));
	t.join();
	return 0;
};

}