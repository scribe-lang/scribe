let io = @import("std/io");
let vec = @import("std/vec");

let Test = struct<T> {
	some: &T;
};

let getElem in Test = fn(idx: any): any {
	return self.some.at(idx);
};

let main = fn(): i32 {
	let v = vec.new(i32, true);
	defer v.deinit();
	for let i = 0; i < 10; ++i {
		v.push(i);
	}
	io.println(v);
	let ref = Test(v){v};
	io.println(ref.getElem(1));
	ref.getElem(1) = 25;
	io.println(ref.some);
	io.println(v);
	return 0;
};
