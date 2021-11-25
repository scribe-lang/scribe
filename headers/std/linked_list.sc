let c = @import("std/c");
let string = @import("std/string");

let Node = struct {
	data: i32;
	next: *self;
};

let newNode = fn(data: i32): *Node {
	let res = c.malloc(Node, 1);
	res.data = data;
	res.next = nil;
	return res;
};

let str in Node = fn() {
	let s = self.data.str();
	defer s.deinit();
	c.puts(s.cStr());
};

let LinkedList = struct {
	start: *Node;
};

let new = fn(): LinkedList {
	return LinkedList{nil};
};

let push in LinkedList = fn(data: i32) {
	let current = self.start;
	if @as(u64, current) == nil {
		self.start = newNode(data);
		return;
	}
	while @as(u64, current.next) != nil {
		current = current.next;
	}
	current.next = newNode(data);
};

let pop in LinkedList = fn() {
	let end = self.start;
	if @as(u64, end) == nil { return; }
	if @as(u64, end.next) == nil {
		c.free(Node, self.start);
		self.start = nil;
		return;
	}
	while @as(u64, end.next.next) != nil {
		end = end.next;
	}
	c.free(Node, end.next);
	end.next = nil;
};

let str in LinkedList = fn() {
	let current = self.start;
	while @as(u64, current) != nil {
		current.str();
		current = current.next;
	}
};

let deinit in LinkedList = fn() {
	let current = self.start;
	while @as(u64, current) != nil {
		let tmp = current;
		current = tmp.next;
		c.free(Node, tmp);
	}
};

inline if @isMainSrc() {

let main = fn(): i32 {
	let ll = new();
	defer ll.deinit();
	for let i = 0; i < 10; ++i {
		ll.push(i);
	}
	ll.str();
	ll.pop();
	ll.push(19);
	ll.str();
	return 0;
};

}