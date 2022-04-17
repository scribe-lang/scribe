let io = @import("std/io");
let err = @import("std/err");
let argparse = @import("std/argparse");

let main = fn(argc: i32, argv: **i8): i32 {
	err.init();
	defer err.deinit();

	let args = argparse.new(argc, argv);
	defer args.deinit();
	args.add(ref"version", ref"v").setHelp(ref"prints program version");
	args.add(ref"value", ref"e").setValReqd(true).setHelp(ref"Enter a value");

	if !args.parse() {
		io.println("Failed to parse command line arguments");
		if err.present() {
			io.println("Error: ", err.pop());
		}
		return 1;
	}
	if args.has(ref"help") {
		args.printHelp(io.stdout);
		return 0;
	}
	if args.has(ref"version") {
		io.println("Scribe Arg Parser v0.0.1, built with Scribe Compiler v", @compilerID());
		return 0;
	}
	if args.has(ref"value") {
		io.println("Value entered: ", args.getArgVal(ref"value"));
	}
	let file = args.getArgIdx(1);
	if file.isEmpty() {
		io.println("No file name provided as argument");
		return 1;
	}
	io.println("Using file: ", file);
	return 0;
};