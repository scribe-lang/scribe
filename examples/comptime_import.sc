let getModule = fn(i: i32): *const i8 {
	if i == 1 {
		return "std/io";
	} else {
		return "std/x";
	}
};

let io = @import(getModule(1));
let comptime y = getModule(0);
let comptime x = getModule(1);

let main = fn(): i32 {
	io.println(x, ", ", y);
	return 0;
};