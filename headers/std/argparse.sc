let c = @import("std/c");
let io = @import("std/io");
let err = @import("std/err");
let map = @import("std/map");
let vec = @import("std/vec");
let string = @import("std/string");

///////////////////////////////////////////////////////////////////////////////////////////////////
// Arg Type
///////////////////////////////////////////////////////////////////////////////////////////////////

let Arg = struct {
	reqd: i1; // is arg required
	val_reqd: i1; // arg takes value or not
	is_present: i1; // is arg present after parsing
	shrt: string.StringRef;
	lng: string.StringRef;
	help: string.StringRef;
	val: string.StringRef;
};

let deinit in Arg = fn() {
	self.shrt.deinit();
	self.lng.deinit();
	self.val.deinit();
	self.help.deinit();
};

let setReqd in Arg = fn(data: i1): &self {
	self.reqd = data;
	return self;
};

let setValReqd in Arg = fn(data: i1): &self {
	self.val_reqd = data;
	return self;
};

let setPresent in Arg = fn(data: i1): &self {
	self.is_present = data;
	return self;
};

let setShort in Arg = fn(data: string.StringRef): &self {
	self.shrt = data;
	return self;
};

let setLong in Arg = fn(data: string.StringRef): &self {
	self.lng = data;
	return self;
};

let setHelp in Arg = fn(data: string.StringRef): &self {
	self.help = data;
	return self;
};

let setVal in Arg = fn(data: string.StringRef): &self {
	self.val = data;
	return self;
};

let getLong in Arg = fn(): &string.StringRef {
	return self.lng;
};

let getShort in Arg = fn(): &string.StringRef {
	return self.shrt;
};

let getVal in Arg = fn(): &string.StringRef {
	return self.val;
};

let getHelp in Arg = fn(): &string.StringRef {
	return self.help;
};

let isReqd in Arg = fn(): i1 {
	return self.reqd;
};

let isValReqd in Arg = fn(): i1 {
	return self.val_reqd;
};

let isPresent in Arg = fn(): i1 {
	return self.is_present;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// ArgParser Type
///////////////////////////////////////////////////////////////////////////////////////////////////

let ArgParser = struct {
	argv: vec.Vec(string.StringRef);
	nooptargs: vec.Vec(string.StringRef);
	args: map.Dict(string.StringRef, Arg);
	shrtargs: map.Dict(string.StringRef, string.StringRef); // short args -> long names
};

let new = fn(argc: i32, argv: **const i8): ArgParser {
	let res = ArgParser{vec.new(string.StringRef, true),
			    vec.new(string.StringRef, true),
			    map.new(string.StringRef, Arg),
			    map.new(string.StringRef, string.StringRef)};
	for let i = 0; i < argc; ++i {
		res.argv.push(string.getRefCStr(argv[i]));
	}
	return res;
};

let deinit in ArgParser = fn() {
	self.argv.deinit();
	self.nooptargs.deinit();
	self.args.deinit();
	self.shrtargs.deinit();
};

let add in ArgParser = fn(data: string.StringRef, shrt: string.StringRef): &Arg {
	self.args.add(data, Arg{false, false, false, shrt, data, ref"", ref""});
	self.shrtargs.add(shrt, data);
	return self.args.get(data);
};

let has in ArgParser = fn(data: string.StringRef): i1 {
	if !self.args.find(data) { return false; }
	return self.args.get(data).isPresent();
};

let getArgIdx in ArgParser = fn(idx: u64): string.StringRef {
	if idx >= self.nooptargs.len() { return ref""; }
	return self.nooptargs[idx];
};

let getArgVal in ArgParser = fn(data: string.StringRef): string.StringRef {
	if !self.args.find(data) { return ref""; }
	return self.args.get(data).getVal();
};

let parse in ArgParser = fn(): i1 {
	let argskeys = self.args.getKeys();
	defer argskeys.deinit();
	let keycount = argskeys.len();

	let expect_key = ref"";
	let expect_val = false;

	for let i: u64 = 0; i < self.argv.len(); ++i {
		let arg = self.argv.getByVal(i);
		if expect_val {
			self.args.get(expect_key).setVal(arg);
			expect_val = false;
			continue;
		}
		if arg.find(ref"--") == 0 {
			arg = arg.subRef(2, 0);
			let found = false;
			if !self.args.find(arg) {
				err.push(1, "Invalid option encountered: --", arg);
				return false;
			}
			let a = self.args.get(arg);
			a.setPresent(true);
			if a.isReqd() { a.setReqd(false); }
			if a.isValReqd() {
				expect_key = a.getLong();
				expect_val = true;
				break;
			}
			continue;
		}
		if arg.find(ref"-") == 0 {
			arg = arg.subRef(1, 0);
			for let i: u64 = 0; i < arg.len(); ++i {
				if !self.shrtargs.find(arg[i].getRef()) {
					err.push(1, "Invalid option encountered: -", arg[i]);
					return false;
				}
				let a = self.args.get(self.shrtargs.get(arg[i].getRef()));
				a.setPresent(true);
				if a.isReqd() { a.setReqd(false); }
				if a.isValReqd() {
					expect_key = a.getLong();
					expect_val = true;
					break;
				}
			}
			continue;
		}
		self.nooptargs.push(arg);
	}
	if expect_val {
		err.push(1, "Value expected for option: --", expect_key);
		return false;
	}
	return true;
};

let printHelp in ArgParser = fn(f: *c.FILE) {
	io.fprint(f, "usage: ", self.argv[0]);
	let argskeys = self.args.getKeys();
	defer argskeys.deinit();

	for let i: u64 = 0; i < argskeys.len(); ++i {
		let a = self.args.get(argskeys[i]);
		if a.isReqd() {
			io.fprint(f, " [", argskeys[i], "]");
		}
	}
	io.fprintln(f, " <args>\n");
	if !argskeys.isEmpty() {
		io.fprintln(f, "Options:");
	}
	for let i: u64 = 0; i < argskeys.len(); ++i {
		let a = self.args.get(argskeys[i]);
		if !a.getShort().isEmpty() {
			io.fprint(f, "-", a.getShort(), ", ");
		}
		io.fprintln(f, "--", a.getLong(), "\t\t", a.getHelp());
	}
};

inline if @isMainSrc() {

let main = fn(argc: i32, argv: **i8): i32 {
	err.init();
	defer err.deinit();

	let args = new(argc, argv);
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

}