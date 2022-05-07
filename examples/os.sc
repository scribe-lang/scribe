let io = @import("std/io");
let os = @import("std/os");
let string = @import("std/string");

let main = fn(): i32 {
	let p = os.getEnv("PATH");
	defer p.deinit();
	if p.isEmpty() {
		io.println("No PATH available");
		return 1;
	}
	io.println("PATH is: ", p);

	let newpath = string.from(p);
	defer newpath.deinit();
	newpath.append(":/tmp/bin");
	os.setEnv("PATH", newpath.cStr(), true);
	io.println("Final PATH is: ", os.getEnv("PATH"));

	let delimited = newpath.delim(':');
	defer delimited.deinit();
	io.println("PATH vector: ", delimited);
	return 0;
};