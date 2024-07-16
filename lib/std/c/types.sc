let core = @import("std/core");

inline if core.currentOS == core.os.Windows {
	let comptime long = i32;
	let comptime ulong = u32;
} else {
	let comptime long = i64;
	let comptime ulong = u64;
}

let comptime int = i32;