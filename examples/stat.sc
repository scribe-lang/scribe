let c = @import("std/c");
let io = @import("std/io");

let stat = c.stat;

let main = fn(argc: i32, argv: **i8): i32 {
	if argc < 2 {
		io.println("Usage: ", argv[0], " <entry path>");
		return 1;
	}
	let st = stat.new();
	let path = argv[1];
	if stat.stat(path, &st) {
		io.println("failed to stat 'stat.sc'");
		return 1;
	}
	if st.isReg() {
		io.println("Regular file: ", path);
	} else {
		io.println("Not a regular file: ", path);
	}
	io.print("Permissions: ");
	if st.isRUSR() { io.print("r"); } else { io.print("-"); }
	if st.isWUSR() { io.print("w"); } else { io.print("-"); }
	if st.isXUSR() { io.print("x"); } else { io.print("-"); }
	if st.isRGRP() { io.print("r"); } else { io.print("-"); }
	if st.isWGRP() { io.print("w"); } else { io.print("-"); }
	if st.isXGRP() { io.print("x"); } else { io.print("-"); }
	if st.isROTH() { io.print("r"); } else { io.print("-"); }
	if st.isWOTH() { io.print("w"); } else { io.print("-"); }
	if st.isXOTH() { io.print("x"); } else { io.print("-"); }
	io.println();
	return 0;
};
