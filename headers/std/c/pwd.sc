let passwd = extern[struct passwd, "<pwd.h>"] struct {
	pw_name: *i8;
	pw_passwd: *i8;
	pw_uid: u32;
	pw_gid: u32;
	pw_gecos: *i8;
	pw_dir: *i8;
	pw_shell: *i8;
};

let getpwuid = extern[getpwuid, "<pwd.h>"] fn(uid: u32): *const passwd;