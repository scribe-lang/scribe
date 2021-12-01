let Node = struct {
	data: i32;
	next: *Self;
};

let FILE = extern[FILE, "<stdio.h>"] struct {
	_flags: i32;
	_IO_read_ptr: *i8;
	// this will generate a warning if instance is created since chain is not the next field in this struct
	chain: *const Self;
	// ...
};

let main = fn(): i32 {
	let n = Node{1, nil};
	return 0;
};