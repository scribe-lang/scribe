let io = @import("std/io");
let os = @import("std/os");

let main = fn(argc: i32, argv: **i8): i32 {
	if argc < 2 {
		io.println("Usage: ", argv[0], " <exe name>");
		return 1;
	}
	let exe = toStringRef(argv[1]);
	let res = os.getExePath(exe);
	defer res.deinit();
	if res.isEmpty() {
		io.println("Exe '", exe, "' does not exist in PATH");
		return 1;
	}
	io.println("Exe found as: ", res);
	return 0;
};