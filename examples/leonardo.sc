// leonardo number

let io = @import("std/io");
let vec = @import("std/vec");

let getNum = fn(from: *const i8): i32 {
	let res = 0;
	while @as(u64, *from) != nil {
		res = 10 * res + ((*from) - '0');
		from = @as(u64, from) + 1;
	}
	return res;
};

let main = fn(argc: i32, argv: **i8): i32 {
	if argc < 2 {
		io.println("Usage: ", argv[0], " <n>");
		return 1;
	}

	let n = getNum(argv[1]);

	let v = vec.new(i32, true);
	defer v.deinit();

	if n >= 2 { v.push(1); --n; }
	if n >= 1 { v.push(1); --n; }

	for let i = 0; i < n; ++i {
		v.push(v[i] + v[i + 1] + 1);
	}
	
	io.println("Result: ", v);
	return 0;
};