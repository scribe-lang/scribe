///////////////////////////////////////////////////////////////////////////////////////////////////
// Iterator for U64
///////////////////////////////////////////////////////////////////////////////////////////////////

let Iter = struct<T, E> {
	of: &T;
	from: E;
	to: E;
	incr: E;
};

let begin in Iter = fn(): self.E {
	return self.from;
};

let end in Iter = fn(): self.E {
	return self.to;
};

let next in Iter = fn(e: self.E): self.E {
	return e + self.incr;
};

let at in Iter = fn(idx: self.E): any {
	return self.of.at(idx);
};