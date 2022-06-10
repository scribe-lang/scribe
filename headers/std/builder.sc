let c = @import("std/c");
let fs = @import("std/fs");
let io = @import("std/io");
let os = @import("std/os");
let err = @import("std/err");
let vec = @import("std/vec");
let string = @import("std/string");
let argparse = @import("std/argparse");

let initArgParser = fn(args: &argparse.ArgParser) {
	args.add(ref"help", ref"h").setHelp(ref"prints program help");
	args.add(ref"debug", ref"d").setHelp(ref"build in debug mode");
	args.add(ref"release", ref"r").setHelp(ref"build in release mode (equivalent to -O 3)");
	args.add(ref"ir", ref"i").setHelp(ref"generate a IR output instead of a binary");
	args.add(ref"llir", ref"l").setHelp(ref"generate LLVM IR output instead of a binary");
	args.add(ref"opt", ref"O").setValReqd(true).setHelp(ref"provide compilation optimization level");
};

let Exe = struct {
	name: string.StringRef;
	path: string.StringRef;
};

let deinit in Exe = inline fn() {};

// All hail the Builder structure
let Builder = struct {
	args: &argparse.ArgParser;
	binaries: vec.Vec(Exe);
};

let new = inline fn(args: &argparse.ArgParser): Builder {
	return Builder{args, vec.new(Exe, true)};
};
let deinit in Builder = inline fn() {
	self.binaries.deinit();
};
let addExe in Builder = fn(name: string.StringRef, path: string.StringRef): self {
	self.binaries.push(Exe{name, path});
	return self;
};

let isDebugBuild in Builder = fn(): i1 {
	if self.args.has(ref"debug") { return true; }
	if self.args.has(ref"release") { return false; }
	if self.args.has(ref"opt") {
		let opt = self.args.getArgVal(ref"opt");
		if opt == ref"0" || opt == ref"g" { return true; }
		return false;
	}
	return true;
};

let isReleaseBuild in Builder = inline fn(): i1 {
	return !self.isDebugBuild();
};

let getOptLevel in Builder = fn(): string.StringRef {
	if self.args.has(ref"debug") { return ref"g"; }
	if self.args.has(ref"release") { return ref"3"; }
	if self.args.has(ref"opt") {
		let opt = self.args.getArgVal(ref"opt");
		if opt != ref"0" && opt != ref"1" && opt != ref"2" && opt != ref"3" &&
		   opt != ref"s" && opt != ref"z" && opt != ref"g" {
			return ref"0";
		}
		return opt;
	}
	return ref"0";
};

let perform in Builder = fn(): i1 {
	// Provided by the scribe compiler when scribe run is executed
	let basedir = self.args.getArgIdx(1).str();
	let currdir = os.getCWD();
	defer basedir.deinit();
	defer currdir.deinit();
	if os.setCWD(basedir.cStr()) {
		err.push(c.errno, "Failed to change dir to: ", basedir, "; error: ", c.strerror(c.errno));
		return false;
	}
	defer os.setCWD(currdir.cStr());

	let tmpres = os.exec("mkdir -p build/rel build/dbg 2>/dev/null");
	if tmpres {
		err.push(tmpres, "Failed to create build directories");
		return false;
	}

	let buildrun = self.args.getArgIdx(2);
	let isdbgbuild = self.isDebugBuild();
	for b in self.binaries.each() {
		let name = b.name;
		let path = b.path.str();
		defer path.deinit();
		if !fs.exists(path.cStr()) {
			err.push(c.errno, "Source path: ", path, " does not exist, or is inaccessible; error: ", c.strerror(c.errno));
			return false;
		}
		let buildtype = ref"Release";
		if isdbgbuild { buildtype = ref"Debug"; }
		io.println("Building: ", name, " (", buildtype, ")");
		let cmd = string.from(@compilerPath(), " -O ", self.getOptLevel(), " ");
		defer cmd.deinit();
		cmd.append(path, " -o build/");
		if isdbgbuild {
			cmd.append("dbg/");
		} else {
			cmd.append("rel/");
		}
		cmd.append(name);
		if self.args.has(ref"ir") {
			cmd.append(" -i");
		} elif self.args.has(ref"llir") {
			cmd.append(" -llir");
		}
		let res = os.exec(cmd);
		if res {
			err.push(res, "Build failed");
			return false;
		}
		if buildrun != ref"run" { continue; }
		cmd.clear();
		cmd.append("./build/");
		if isdbgbuild {
			cmd.append("dbg/");
		} else {
			cmd.append("rel/");
		}
		cmd.append(name);
		res = os.exec(cmd);
		if res {
			err.push(res, "Execution failed");
			return false;
		}
	}
	return true;
};
