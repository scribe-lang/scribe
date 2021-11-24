let Node = struct {
	data: i32;
	next: *self;
};

let n = Node{1, nil};

let FILE = extern[FILE, "<stdio.h>"] struct {
	_flags: i32;
	_IO_read_ptr: *i8;
	// this will generate a warning since chain is not the next field in this struct
	chain: *const self;
	// ...
};

let f = FILE{1, nil, nil};