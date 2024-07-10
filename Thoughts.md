The "associated" functions should just be a syntactic sugar for regular functions with self as first parameter.
Programmatically, this is what it means:

```rs
let someFn in someStruct = fn(other: someOtherStruct) {
	// ...
};
```

is equivalent to:

```rs
let someFn = fn(self: &someStruct, other: someOtherStruct) {
	// ...
};
```

This will require implementation of function overloading based on parameters.
This should be fine anyway considering that template functions already exist.

////////////////////////////////////////////////

Everything must be inside a "basic block"

This is also how jump operations (conditional constructs) and loop constructs know where to jump to.