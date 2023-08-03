let global StringRef = struct {
	start: *const i8;
	count: u64;
};

let deinit in StringRef = inline fn() {};