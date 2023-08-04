let io = @import("std/io");

let main = fn(): i32 {
	let f = io.fopen(r"/tmp/xyz", r"w+");
	// multiple defers follows LIFO order
	// flush() is only for illustration, not used before close() in real life
	defer f.close();
	defer f.flush();
	f.println("result of 1 + 2 = ", 1 + 2);
	f.println("another line");
	return 0;
};