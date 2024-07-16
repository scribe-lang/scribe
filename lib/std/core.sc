// this file does not depend on any other

// list of operating system names
let os = enum : i8 {
	Unknown,
	Linux,
	Windows,
	Apple,
	Android,
	FreeBSD,
	NetBSD,
	OpenBSD,
	DragonFly,
};

let comptime currentOS = @getOSID();
