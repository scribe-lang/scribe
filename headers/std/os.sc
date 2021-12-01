let c = @import("std/c");
let string = @import("std/string");

let getEnv = fn(key: *const i8): *const i8 {
	return c.getenv(key);
};
let setEnv = fn(key: *const i8, val: *const i8, overwrite: i1): i32 {
	return c.setenv(key, val, overwrite);
};