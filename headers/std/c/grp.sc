let group = extern[struct group, "<grp.h>"] struct {
	gr_name: *i8;
	gr_passwd: *i8;
	gr_gid: u32;
	gr_mem: **i8;
};

let getgrgid = extern[getgrgid, "<grp.h>"] fn(uid: u32): *const group;