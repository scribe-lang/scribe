let stdin = 0;
let stdout = 1;
let stderr = 2;

let FILE = extern[FILE, "<stdio.h>"] struct {
	_flags: i32;
	_IO_read_ptr: *i8;
	// ...
};