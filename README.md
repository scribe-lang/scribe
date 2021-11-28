# About

Scribe is a statically typed, compiled (currently, to C), procedural systems programming langauge with a focus on minimalism, simplicity, and utility.

It attempts to find a balance between the complexities of C++ and the barebones nature of C. As such, it is mostly inspired by C, while containing the member function syntax like that in C++.
There are also features taken from the Zig programming language.

## Examples

1. Hello World

```rs
let io = @import("std/io");

let main = fn(): i32 {
	io.println("Hello World");
	return 0;
};
```

2. A Basic Function

```rs
let io = @import("std/io");

let func = fn(data: *const i8) {
	io.println("Hello ", data);
};

let main = fn(): i32 {
	func("Electrux");
	return 0;
};
```

3. Factorial of 5

```rs
let io = @import("std/io");

let facto = fn(data: i32): i32 {
	let f = 1;
	for let i = data; i > 1; --i {
		f *= i;
	}
	return f;
};

let main = fn(): i32 {
	io.println("Factorial of 5 is: ", facto(5));
	return 0;
};
```

4. Structures And Member Functions

There are no classes. Only structs. And even though member functions are present, they are not encapsulated within structs.
Instead, they can be freely created across the code and used anywhere after the declaration.

This essentially allows one to write custom member functions on existing structs and therefore augment is as desired.

```rs
let io = @import("std/io");
let string = @import("std/string");

let Point = struct {
	x: i32;
	y: u32;
	z: f64;
};

let str in const Point = fn(): string.String {
	let str = string.new();
	str.appendCStr("{");
	str.appendInt(self.x);
	str.appendCStr(", ");
	str.appendUInt(self.y);
	str.appendCStr(", ");
	str.appendFlt(self.z);
	str.appendCStr("}");
	return str;
};

let main = fn(): i32 {
	let point = Point{1, 2, 3.25};
	io.println("Point: ", point);
	// another way:
	let str = point.str();
	defer str.deinit();
	io.println("Point: ", str);
	return 0;
};
```