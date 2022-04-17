// Basic error handing:
// Used to propagate errors across functions
// without using return values or args

let vec = @import("std/vec");
let string = @import("std/string");

let static inited = false;

let Error = struct {
	code: i32;
	msg: string.StringRef;
};

let deinit in Error = fn() {
	self.msg.deinit();
};

let str in const Error = fn(): string.String {
	let s = string.from("{ Code: ");
	s.appendInt(self.code);
	s.appendCStr(", Message: ", 0);
	s.appendRef(self.msg);
	s.appendCStr(" }", 2);
	return s;
};

let getStack = fn(): &vec.Vec(Error) {
	let static errstack: vec.Vec(Error);
	return errstack;
};

let getStrStack = fn(): &vec.Vec(string.String) {
	let static strstack: vec.Vec(string.String);
	return strstack;
};

let init = fn() {
	if inited { return; }
	getStack() = vec.new(Error, true);
	getStrStack() = vec.new(string.String, true);
	inited = true;
};

let deinit = fn() {
	getStack().deinit();
	getStrStack().deinit();
	inited = false;
};

let allToRef = fn(data: ...&const any): string.StringRef {
	let comptime len = @valen();
	inline if len == 1 && @isCString(data[0]) {
		return string.getRefCStr(data[0]);
	}
	inline if len == 1 && @isEqualTy(data[0], string.String) {
		return data[0].getRef();
	}
	inline if len == 1 && @isEqualTy(data[0], string.StringRef) {
		return data[0];
	}
	let errstr = string.new();
	inline for let comptime i = 0; i < len; ++i {
		inline if @isCString(data[i]) {
			errstr.appendCStr(data[i], 0);
		} elif @isCChar(data[i]) {
			errstr.appendChar(data[i]);
		} elif @isEqualTy(data[i], string.String) {
			errstr += data[i];
		} elif @isEqualTy(data[i], string.StringRef) {
			errstr.appendRef(data[i]);
		} else {
			let s = data[i].str();
			defer s.deinit();
			errstr += s;
		}
	}
	getStrStack().push(errstr);
	return errstr.getRef();
};

let push = fn(code: i32, data: ...&const any) {
	if !inited { return; }
	let e = Error{code, allToRef(data)};
	getStack().push(e);
};

let present = fn(): i1 {
	if !inited { return false; }
	return !getStack().isEmpty();
};

let getCode = fn(): i32 {
	if !inited { return -1; }
	if getStack().empty() { return -1; }
	return getStack().back().code;
};

let getMsg = fn(): string.StringRef {
	if !inited { return ref"err system uninitialized"; }
	if getStack().empty() {
		return ref"no error present in stack";
	}
	return getStack().back().msg;
};

let pop = fn(): Error {
	if !inited { return Error{-1, ref"err system uninitialized"}; }
	if !getStack().isEmpty() {
		let res = getStack().backByVal();
		getStack().pop();
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