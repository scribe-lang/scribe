let x = 5;

let add = fn(a: i32, b: i32): i32 {
	return a + b;
};

let st = struct {
	a: i32;
	b: i64;
	c: i1;
};

let __assn__ in st = fn(other: &const st): &st {
	self.a = other.a;
	self.b = other.b;
	self.c = other.c;
	return self;
};