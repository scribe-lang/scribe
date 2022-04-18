let unistd = @import("std/unistd");

let pthread_t: u64 = extern[pthread_t, "<pthread.h>", "-pthread"];
let pthread_attr_t = extern[pthread_attr_t, "<pthread.h>", "-pthread"] struct {};

let pthread_create = extern[pthread_create, "<pthread.h>", "-pthread"] fn(id: *pthread_t, attr: *const pthread_attr_t, start_routine: fn(data: *void): *void, arg: *void): i32;
let pthread_join = extern[pthread_join, "<pthread.h>", "-pthread"] fn(id: pthread_t, value_ptr: **void): i32;

let getSelf = extern[pthread_self, "<pthread.h>", "-pthread"] fn(): pthread_t;

let getConcurrency = fn(): i64 {
	return unistd.sysconf(unistd._SC_NPROCESSORS_ONLN);
};

let Thread = struct {
	threadid: pthread_t;
};

let new = fn(): Thread {
	return Thread{0};
};

let getID in Thread = fn(): pthread_t {
	return self.threadid;
};

let run in Thread = fn(routine: fn(data: *void): *void, arg: *void): i32 {
	return pthread_create(&self.threadid, nil, routine, arg);
};

let join in Thread = fn(): i32 {
	return pthread_join(self.threadid, nil);
};

// usage
inline if @isMainSrc() {

let io = @import("std/io");

let func = fn(data: *void): *void {
	let tid = @as(u64, data);
	io.println("Hello world from thread ", getSelf(), " with data: ", tid);
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