
let init = fn(comptime T: type): Data(T) {
	return Data(T){nil};
};

// IR:

beginSignature [1, FUNC, false] // [ArgCount (template inclusive), Func/Struct/Union, IsExtern]
	load [IDEN, "type"]
	createVar ["T", false, true, false, true, false] // [Name: str, In: bool, Type: bool, Val: bool, Comptime: bool, Variadic: bool]
	load [IDEN, "void"]
endSignature [] // Pushes the signature in the stack. If it's a func, top of stack is taken as return type before pushing the signature.
beginFunc [] // Picks the top of stack as signature and loads it (so args are available and return type can be checked)
	load [IDEN "nil"] // struct instantiation value
	load [IDEN, "T"]
	load [IDEN, "Data"]
	call [1, false, false, false] // [ArgCount, IsIntrinsic, IsStructInstantiate, IsMemberCall]
	call [1, false, true, false]
	ret [TRUE] // [HasArg]; If has arg, it is taken from stack
endFunc [] // Now the function definition (value) is stored in stack, with the signature as type
createVar ["init", false, false, true, false, false]
