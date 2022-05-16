let io = @import("std/io");
let os = @import("std/os");

let getOSName = fn(): *const i8 {
	inline if os.currentOS == os.id.Linux {
		return "Using Linux";
	} elif os.currentOS == os.id.Windows {
		return "Using Windows";
	} elif os.currentOS == os.id.Apple {
		return "Using Apple's OS";
	} elif os.currentOS == os.id.Android {
		return "Using Android";
	} else {
		return "Using a rare OS";
	}
};

let TempEnum = enum : i16 {
	A,
	B,
	C
};

// TODO: for now there can be no semblence of templates in main() function as it will be ignored by the type system
// this needs to be fixed - type system will have to consider main to be an exception to the ignore rule
let main = fn(): i32 {
	io.println(getOSName());
	let comptime x = @enumTagTy(TempEnum); // x = i16
	let y: x; // y: i16
	let z = TempEnum.A; // z: i16 = 0
	return 0;
};