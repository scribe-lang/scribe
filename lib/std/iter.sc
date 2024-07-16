///////////////////////////////////////////////////////////////////////////////////////////////////
// Iterator for U64
///////////////////////////////////////////////////////////////////////////////////////////////////

let Iter = struct<T, E> {
	of: &T;
	from: E;
	to: E;
	incr: E;
};

let begin in Iter = inline fn(): self.E {
	return self.from;
};

let end in Iter = inline fn(): self.E {
	return self.to;
};

let next in Iter = inline fn(e: self.E): self.E {
	return e + self.incr;
};

let at in Iter = inline fn(idx: self.E): any {
	return self.of.at(idx);
};