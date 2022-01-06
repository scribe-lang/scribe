///////////////////////////////////////////////////////////////////////////////////////////////////
// Iterator for U64
///////////////////////////////////////////////////////////////////////////////////////////////////

let U64Iter = struct<T> {
	of: &T;
	from: u64;
	to: u64;
	incr: i64;
};

let begin in U64Iter = fn(): u64 {
	return self.from;
};

let end in U64Iter = fn(): u64 {
	return self.to;
};

let next in U64Iter = fn(e: u64): u64 {
	return e + self.incr;
};

let at in U64Iter = fn(idx: u64): &any {
	return self.of.at(idx);
};