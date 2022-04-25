let os = @import("std/os");

inline if os.currentOS == os.id.Linux {

let io = @import("std/io");

}

let main = fn(): i32 {
	io.println("Hello world");
	return 0;
};