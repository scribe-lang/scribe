let Mutex = extern[pthread_mutex_t, "<pthread.h>", "-pthread"] struct {};
let MutexAttr = extern[pthread_mutexattr_t, "<pthread.h>", "-pthread"] struct {};

let mutex_init = extern[pthread_mutex_init, "<pthread.h>", "-pthread"] fn(mtx: *Mutex, value_ptr: *MutexAttr): i32;
let mutex_lock = extern[pthread_mutex_lock, "<pthread.h>", "-pthread"] fn(mtx: *Mutex): i32;
let mutex_unlock = extern[pthread_mutex_unlock, "<pthread.h>", "-pthread"] fn(mtx: *Mutex): i32;
let mutex_destroy = extern[pthread_mutex_destroy, "<pthread.h>", "-pthread"] fn(mtx: *Mutex): i32;

let new = fn(): Mutex {
	let mtx: Mutex;
	return mtx;
};

let init in Mutex = inline fn(): i1 {
	return mutex_init(&self, nil) == 0;
};

let lock in Mutex = inline fn(): i1 {
	return mutex_lock(&self) == 0;
};

let unlock in Mutex = inline fn(): i1 {
	return mutex_unlock(&self) == 0;
};

let deinit in Mutex = inline fn(): i1 {
	return mutex_destroy(&self) == 0;
};

inline if @isMainSrc() {

let io = @import("std/io");
let thread = @import("std/thread");

let mtx: Mutex;

let func = fn(data: *void): *void {
	let tid = @as(@ptr(u64), data);
	mtx.lock();
	io.println("Hello world from thread ", thread.getSelf(), " with data: ", *tid);
	mtx.unlock();
	return nil;
};

let main = fn(): i32 {
	mtx.init();
	defer mtx.deinit();

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

}