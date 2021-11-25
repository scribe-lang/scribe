let c = @import("std/c");
let linked_list = @import("std/linked_list");

let main = fn(): i32 {
	let ll = linked_list.new();
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