// this is a prelude file - it is ALWAYS imported before the invoked program

// StringRef must be defined in scribe code, hence the intrinsic to register with compiler.
let global StringRef = struct{
	data: *const i8;
	length: u64;
};
@setStringRefTy(StringRef);

let deinit in StringRef = inline fn() {};