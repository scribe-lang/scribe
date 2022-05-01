let ctype = @import("std/c/types");

let Stat = extern[struct stat, "<sys/stat.h>"] struct {
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

let new = fn(): Stat {
	return Stat{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, @array(ctype.long, 3)};
};

let stat = extern[stat, "<sys/stat.h>"] fn(pathname: *const i8, statbuf: *Stat): ctype.int;
let fstat = extern[fstat, "<sys/stat.h>"] fn(fd: ctype.int, statbuf: *Stat): ctype.int;
let lstat = extern[lstat, "<sys/stat.h>"] fn(pathname: *const i8, statbuf: *Stat): ctype.int;
let fstatat = extern[lstat, "<sys/stat.h>"] fn(dirfd: ctype.int, statbuf: *Stat, flags: ctype.int): ctype.int;

// usage: (st_mode & S_IFMT) == <x>

let S_IFMT: i32 = extern[S_IFMT, "<sys/stat.h>"];
let S_IFSOCK: i32 = extern[S_IFSOCK, "<sys/stat.h>"];
let S_IFLNK: i32 = extern[S_IFLNK, "<sys/stat.h>"];
let S_IFREG: i32 = extern[S_IFREG, "<sys/stat.h>"];
let S_IFBLK: i32 = extern[S_IFBLK, "<sys/stat.h>"];
let S_IFDIR: i32 = extern[S_IFDIR, "<sys/stat.h>"];
let S_IFCHR: i32 = extern[S_IFCHR, "<sys/stat.h>"];
let S_IFIFO: i32 = extern[S_IFIFO, "<sys/stat.h>"];

// usage: st_mode & <x>

let S_ISUID: i32 = extern[S_ISUID, "<sys/stat.h>"];
let S_ISGID: i32 = extern[S_ISGID, "<sys/stat.h>"];
let S_ISVTX: i32 = extern[S_ISVTX, "<sys/stat.h>"];
// user
let S_IRWXU: i32 = extern[S_IRWXU, "<sys/stat.h>"];
let S_IRUSR: i32 = extern[S_IRUSR, "<sys/stat.h>"];
let S_IWUSR: i32 = extern[S_IWUSR, "<sys/stat.h>"];
let S_IXUSR: i32 = extern[S_IXUSR, "<sys/stat.h>"];
// group
let S_IRWXG: i32 = extern[S_IRWXG, "<sys/stat.h>"];
let S_IRGRP: i32 = extern[S_IRGRP, "<sys/stat.h>"];
let S_IWGRP: i32 = extern[S_IWGRP, "<sys/stat.h>"];
let S_IXGRP: i32 = extern[S_IXGRP, "<sys/stat.h>"];
// other
let S_IRWXO: i32 = extern[S_IRWXO, "<sys/stat.h>"];
let S_IROTH: i32 = extern[S_IROTH, "<sys/stat.h>"];
let S_IWOTH: i32 = extern[S_IWOTH, "<sys/stat.h>"];
let S_IXOTH: i32 = extern[S_IXOTH, "<sys/stat.h>"];

let isReg in const Stat = fn(): i1 { return (self.st_mode & S_IFMT) == S_IFREG; };
let isDir in const Stat = fn(): i1 { return (self.st_mode & S_IFMT) == S_IFDIR; };
let isChr in const Stat = fn(): i1 { return (self.st_mode & S_IFMT) == S_IFCHR; };
let isBlk in const Stat = fn(): i1 { return (self.st_mode & S_IFMT) == S_IFBLK; };
let isFifo in const Stat = fn(): i1 { return (self.st_mode & S_IFMT) == S_IFIFO; };
let isLink in const Stat = fn(): i1 { return (self.st_mode & S_IFMT) == S_IFLNK; };
let isSock in const Stat = fn(): i1 { return (self.st_mode & S_IFMT) == S_IFSOCK; };

let isUID in const Stat = fn(): i1 { return self.st_mode & S_ISUID; };
let isGID in const Stat = fn(): i1 { return self.st_mode & S_ISGID; };
let isVTX in const Stat = fn(): i1 { return self.st_mode & S_ISVTX; };

let isRWXU in const Stat = fn(): i1 { return self.st_mode & S_IRWXU; };
let isRUSR in const Stat = fn(): i1 { return self.st_mode & S_IRUSR; };
let isWUSR in const Stat = fn(): i1 { return self.st_mode & S_IWUSR; };
let isXUSR in const Stat = fn(): i1 { return self.st_mode & S_IXUSR; };

let isRWXG in const Stat = fn(): i1 { return self.st_mode & S_IRWXG; };
let isRGRP in const Stat = fn(): i1 { return self.st_mode & S_IRGRP; };
let isWGRP in const Stat = fn(): i1 { return self.st_mode & S_IWGRP; };
let isXGRP in const Stat = fn(): i1 { return self.st_mode & S_IXGRP; };

let isRWXO in const Stat = fn(): i1 { return self.st_mode & S_IROTH; };
let isROTH in const Stat = fn(): i1 { return self.st_mode & S_IROTH; };
let isWOTH in const Stat = fn(): i1 { return self.st_mode & S_IWOTH; };
let isXOTH in const Stat = fn(): i1 { return self.st_mode & S_IXOTH; };

