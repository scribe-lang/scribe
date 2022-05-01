let ctype = @import("std/c/types");

let stat = extern[struct stat, "<sys/stat.h>"] struct {
	st_dev: u64;
	st_ino: u64;
	st_nlink: u64;
	st_mode: u32;
	st_uid: u32;
	st_gid: u32;
	pad: ctype.int;
	st_rdev: u64;
	st_size: i64;
	st_blksize: i64;
	st_blocks: i64;
	st_atime: i64;
	st_atimensec: ctype.long;
	st_mtime: i64;
	st_mtimensec: ctype.long;
	st_ctime: i64;
	st_ctimensec: ctype.long;
	__glibc_reserved1: @array(ctype.long, 3);
};


