let io = @import("std/io");
let str = @import("std/str");

let main = fn(): i32 {
	let cstr = "c like string"; // type = *const i8
	let str1 = str.new("object string"); // type(struct) = str.Str
	defer str1.deinit();

	let str2 = str1.copy(); // deep copy
	str2.appendCStr(" added to object string"); // calls += operator with *const i8 as argument

	let str3 = str2 + str2; // calls + operator with str.Str as argument; overloading is NOT allowed for multiple types
	defer str3.deinit();

	let str5: str.Str = str.new(); // all types/structs are instantiated using their respective 'new'/'create' functions
	let str6: *str.Str = nil; // pointers yay! pointers are always initialized to nil, if they don't have a value, by compiler
	str6 = &str2; // assign to pointer
	str6.clear(); // (pointer) member function call; clear() must set the internal dynamic array to nil so that deinit() works properly
	str6[0] = '5'; // calls [] operator which returns an &i8
	return 0;
};