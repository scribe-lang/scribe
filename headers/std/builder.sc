let c = @import("std/c");
let fs = @import("std/fs");
let io = @import("std/io");
let os = @import("std/os");
let err = @import("std/err");
let vec = @import("std/vec");
let string = @import("std/string");
let argparse = @import("std/argparse");

let initArgParser = fn(args: &argparse.ArgParser) {
	args.add("help", "h").setHelp("prints program help");
	args.add("debug", "d").setHelp("build in debug mode");
	args.add("release", "r").setHelp("build in release mode (equivalent to -O 3)");
	args.add("ir", "i").setHelp("generate a IR output instead of a binary");
	args.add("llir", "l").setHelp("generate LLVM IR output instead of a binary");
	args.add("opt", "O").setValReqd(true).setHelp("provide compilation optimization level");
};

let Exe = struct {
	name: StringRef;
	path: StringRef;
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
let addExe in Builder = fn(name: StringRef, path: StringRef): self {
	self.binaries.push(Exe{name, path});
	return self;
};

let isDebugBuild in Builder = fn(): i1 {
	if self.args.has("debug") { return true; }
	if self.args.has("release") { return false; }
	if self.args.has("opt") {
		let opt = self.args.getArgVal("opt");
		if opt == "0" || opt == "g" { return true; }
		return false;
	}
	return true;
};

let isReleaseBuild in Builder = inline fn(): i1 {
	return !self.isDebugBuild();
};

let getOptLevel in Builder = fn(): StringRef {
	if self.args.has("debug") { return "g"; }
	if self.args.has("release") { return "3"; }
	if self.args.has("opt") {
		let opt = self.args.getArgVal("opt");
		if opt != "0" && opt != "1" && opt != "2" && opt != "3" &&
		   opt != "s" && opt != "z" && opt != "g" {
			return "0";
		}
		return opt;
	}
	return "0";
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
		let buildtype = "Release";
		if isdbgbuild { buildtype = "Debug"; }
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
		if self.args.has("ir") {
			cmd.append(" -i");
		} elif self.args.has("llir") {
			cmd.append(" -llir");
		}
		let res = os.exec(cmd);
		if res {
			err.push(res, "Build failed");
			return false;
		}
		if buildrun != "run" { continue; }
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
