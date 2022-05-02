let ctype = @import("std/c/types");

let dirent = extern[struct dirent, "<dirent.h>"] struct {
	d_ino: ctype.ulong;
	d_off: ctype.long;
	d_reclen: u16;
	d_type: u8;
	d_name: @array(i8, 256);
};

let DT_UNKNOWN: u8 = extern[DT_UNKNOWN, "<dirent.h>"];
let DT_REG: u8 = extern[DT_REG, "<dirent.h>"];
let DT_DIR: u8 = extern[DT_DIR, "<dirent.h>"];
let DT_FIFO: u8 = extern[DT_FIFO, "<dirent.h>"];
let DT_SOCK: u8 = extern[DT_SOCK, "<dirent.h>"];
let DT_CHR: u8 = extern[DT_CHR, "<dirent.h>"];
let DT_BLK: u8 = extern[DT_BLK, "<dirent.h>"];
let DT_LNK: u8 = extern[DT_LNK, "<dirent.h>"];

let DIR = extern[DIR, "<dirent.h>"] struct {};

let opendir = extern[opendir, "<dirent.h>"] fn(dirname: *const i8): *DIR;
let closedir = extern[closedir, "<dirent.h>"] fn(dirp: *DIR): i32;
let readdir = extern[readdir, "<dirent.h>"] fn(dirp: *DIR): *dirent;
let rewinddir = extern[rewinddir, "<dirent.h>"] fn(dirp: *DIR);
let seekdir = extern[seekdir, "<dirent.h>"] fn(dirp: *DIR, loc: ctype.long);
let telldir = extern[rewinddir, "<dirent.h>"] fn(dirp: *DIR): ctype.long;