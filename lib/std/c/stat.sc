let ctype = @import("std/c/types");

let Stat = extern[struct stat, "<sys/stat.h>"] struct {
	st_dev: u64;
	st_ino: u64;
	st_nlink: u64;
	st_mode: u32;
	st_uid: u32;
	st_gid: u32;
	__pad0: ctype.int;
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
	__glibc_reserved: @array(ctype.long, 3);
};

let new = fn(): Stat {
	return Stat{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, @array(ctype.long, 3)};
};

let deinit in Stat = fn() {};

let copy in Stat = fn(other: &const Stat) {
	self.st_dev = other.st_dev;
	self.st_ino = other.st_ino;
	self.st_nlink = other.st_nlink;
	self.st_mode = other.st_mode;
	self.st_uid = other.st_uid;
	self.st_gid = other.st_gid;
	self.__pad0 = other.__pad0;
	self.st_rdev = other.st_rdev;
	self.st_size = other.st_size;
	self.st_blksize = other.st_blksize;
	self.st_blocks = other.st_blocks;
	self.st_atime = other.st_atime;
	self.st_atimensec = other.st_atimensec;
	self.st_mtime = other.st_mtime;
	self.st_mtimensec = other.st_mtimensec;
	self.st_ctime = other.st_ctime;
	self.st_ctimensec = other.st_ctimensec;
	self.__glibc_reserved[0] = other.__glibc_reserved[0];
	self.__glibc_reserved[1] = other.__glibc_reserved[1];
	self.__glibc_reserved[2] = other.__glibc_reserved[2];
};

let clear in Stat = fn() {
	self.st_dev = 0;
	self.st_ino = 0;
	self.st_nlink = 0;
	self.st_mode = 0;
	self.st_uid = 0;
	self.st_gid = 0;
	self.__pad0 = 0;
	self.st_rdev = 0;
	self.st_size = 0;
	self.st_blksize = 0;
	self.st_blocks = 0;
	self.st_atime = 0;
	self.st_atimensec = 0;
	self.st_mtime = 0;
	self.st_mtimensec = 0;
	self.st_ctime = 0;
	self.st_ctimensec = 0;
	self.__glibc_reserved[0] = 0;
	self.__glibc_reserved[1] = 0;
	self.__glibc_reserved[2] = 0;
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

