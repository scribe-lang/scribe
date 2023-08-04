let os = @import("std/os");

let getModule = fn(): StringRef {
	if os.currentOS == os.id.Linux {
		return "std/io";
	} elif os.currentOS == os.id.Apple {
		return "std/io_apple";
	}
	return "";
};

let io = @import(getModule());
let comptime io_name = getModule();

let main = fn(): i32 {
	io.println(io_name);
	return 0;
};