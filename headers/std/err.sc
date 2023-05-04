// Basic error handing:
// Used to propagate errors across functions
// without using return values or args

let vec = @import("std/vec");
let list = @import("std/list");
let string = @import("std/string");

let Error = struct {
	code: i32;
	msg: string.StringRef;
};

let deinit in Error = fn() {
	self.msg.deinit();
};

let str in const Error = fn(): string.String {
	let s = string.from("{ Code: ");
	s.append(self.code); // internally calls s.appendInt()
	s.append(", Message: ");
	s.append(self.msg); // internally calls s.appendRef()
	s.appendCStr(" }", 2);
	return s;
};

let static inited = false;
let static errstack: vec.Vec(Error);
let static strlist: list.List(string.String);

let init = fn() {
	if inited { return; }
	errstack.init(true);
	strlist.init(true);
	inited = true;
};

let deinit = fn() {
	if !inited { return; }
	errstack.deinit();
	strlist.deinit();
	inited = false;
};

let allToRef = fn(data: ...&const any): string.StringRef {
	let comptime len = @valen();
	inline if len == 1 && @isCString(data[0]) {
		return string.getRefCStr(data[0]);
	} elif len == 1 && @isEqualTy(data[0], string.String) {
		return data[0].getRef();
	} elif len == 1 && @isEqualTy(data[0], string.StringRef) {
		return data[0];
	} else {
		let errstr = string.new();
		inline for let comptime i = 0; i < len; ++i {
			errstr.append(data[i]); // String.append() is a generic function
		}
		strlist.push(errstr);
		return strlist.back().getRef();
	}
};

let push = fn(code: i32, data: ...&const any) {
	if !inited { return; }
	let e = Error{code, allToRef(data)};
	errstack.push(e);
};

let present = fn(): i1 {
	if !inited { return false; }
	return !errstack.isEmpty();
};

let getCode = fn(): i32 {
	if !inited { return -1; }
	if errstack.isEmpty() { return -1; }
	return errstack.back().code;
};

let getMsg = fn(): string.StringRef {
	if !inited { return ref"err system uninitialized"; }
	if errstack.isEmpty() {
		return ref"no error present in stack";
	}
	return errstack.back().msg;
};

let pop = fn(): Error {
	if !inited { return Error{-1, ref"err system uninitialized"}; }
	if !errstack.isEmpty() {
		let res = errstack.backByVal();
		errstack.pop();
		return res;
	}
	return Error{-1, ref"invalid error pop"};
};

inline if @isMainSrc() {

let io = @import("std/io");

let main = fn(): i32 {
	init();
	defer deinit();

	push(5, ref"error code 5");
	push(3, ref"error code 3");
	push(1, ref"error code 1");
	push(2, ref"error code 2");

	io.println(pop());
	io.println(pop());
	io.println(pop());
	io.println(pop());
	io.println(pop());
	return 0;
};

}