/*
 * Doubly Linked List implementation
 */

let mem = @import("std/mem");
let string = @import("std/string");

let Node = struct<T> {
	data: T;
	prev: *Self;
	next: *Self;
};

let List = struct<T> {
	start: *Node(T);
	end: *Node(T);
	length: u64;
	managed: i1;
};

let init in List = fn(managed: i1) {
	self.start = nil;
	self.end = nil;
	self.length = 0;
	self.managed = managed;
};

let deinit in List = fn() {
	while @as(u64, self.start) != nil {
		let tmp = self.start;
		self.start = self.start.next;
	inline if !@isPrimitive(self.T) {
		if self.managed { tmp.data.deinit(); }
	}
		mem.free(Node(self.T), tmp);
	}
	self.start = self.end = nil;
	self.length = 0;
};

let new = inline fn(comptime T: type, managed: i1): List(T) {
	return List(T){nil, nil, 0, managed};
};

let push in List = fn(data: &const self.T): self {
	let newnode = mem.alloc(Node(self.T), 1);
	newnode.data = data;
	newnode.prev = nil;
	newnode.next = nil;
	if @as(u64, self.start) == nil {
		self.start = self.end = newnode;
	} else {
		self.end.next = newnode;
		newnode.prev = self.end;
		self.end = self.end.next;
	}
	++self.length;
	return self;
};

let pushVal in List = inline fn(data: self.T): self {
	return self.push(data);
};

let pop in List = fn(): self {
	if @as(u64, self.start) == nil { return self; }
	--self.length;
	inline if !@isPrimitive(self.T) {
		if self.managed { self.end.data.deinit(); }
	}
	if @as(u64, self.start) == @as(u64, self.end) {
		mem.free(Node(self.T), self.end);
		self.start = self.end = nil;
		return self;
	}
	let tmp = self.end;
	self.end = self.end.prev;
	self.end.next = nil;
	mem.free(Node(self.T), tmp);
	return self;
};

let clear in List = inline fn() {
	let tmp = self.managed;
	self.deinit();
	self.init(tmp);
};

let setManaged in List = fn(managed: i1): self {
	self.managed = managed;
	return self;
};

let doEach in List = fn(cb: any, args: ...&any) {
	let it = self.start;
	while @as(u64, it) != nil {
		cb(it.data, args);
		it = it.next;
	}
};

let front in List = inline fn(): &self.T { return self.start.data; };
let frontByVal in const List = inline fn(): self.T { return self.start.data; };
let back in List = inline fn(): &self.T { return self.end.data; };
let backByVal in const List = inline fn(): self.T { return self.end.data; };

let len in const List = inline fn(): u64 { return self.length; };
let isEmpty in const List = inline fn(): i1 { return @as(u64, self.start) == nil; };

let str in const List = fn(): string.String {
	let res = string.from("[");
	let tmp = self.start;
	for let i = 0; i < self.length; ++i {
		res += tmp.data;
		tmp = tmp.next;
		if i < self.length - 1 { res.appendRef(", "); }
	}
	res.appendRef("]");
	return res;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Usage
///////////////////////////////////////////////////////////////////////////////////////////////////

inline if @isMainSrc() {

let io = @import("std/io");
let list = @import("std/list");

let main = fn(): i32 {
	let l = new(i32, true);
	defer l.deinit();
	for let i = 0; i < 10; ++i {
		l.push(i);
		io.println("count: ", l.len(), "; list: ", l);
	}
	for let i = 0; i < 5; ++i {
		l.pop();
		io.println("count: ", l.len(), "; list: ", l);
	}
	return 0;
};

}