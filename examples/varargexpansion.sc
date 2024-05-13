let X = struct {
	i: i32;
	j: StringRef;
};

let f = fn(args: ...any): X {
	return X{args...};
};

let main = fn(): i32 {
	return f(5, "lol").i;
};