let access = extern[access, "<unistd.h>"] fn(path: *const i8, amode: i32): i32;
let F_OK: i32 = extern[F_OK, "<unistd.h>"];

let exists = fn(path: *const i8): i1 {
	return access(path, F_OK) != -1;
};