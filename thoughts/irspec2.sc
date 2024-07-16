let io = @import("std/io");

let Data = struct(comptime T: type, list: *T);

let init = fn(comptime T: type): Data(T) {
	return Data(T){nil};
};

let __subscr__ in Data = fn(idx: u64): &self.T {
	return self.list[idx];
};

let str in const Data = fn(): *const i8 {
	return "Hi";
};

let main = fn(): i32 {
	let data = init(i32);
	let d = 0;
	for let i = 0; i < 10; ++i {
		if i == 5 { continue; }
		d += data[i];
	}
	io.println(d);
	return 0;
};

// IR:
// Note: Indentation here is only for viewing pleasure. It does not exist in code.

load [STR, "std/io"]
load [IDEN, "import"]

call [TRUE, 1] // [IsInstrin, ArgCount]
createVar ["io", false, false, true, false, false] // [Name: string, In: TypeValInstr/nil, Type: TypeValInstr/nil, val: Instr/nil, comptime: bool, variadic: bool]

beginStruct [1, "T"] // [FieldCount, Templates...]
	load [IDEN, "T"]
	load [IDEN, "typePtr"]
	call [TRUE, 1]
	createVar ["list", false, true, false, false, false]
endStruct []
createVar ["Data", false, false, true, false, false]

beginSignature [1, TRUE] // [ArgCount, isFunc (or struct/union)]
	load [IDEN, "type"]
	createVar ["T", false, true, false, true, false] // This will be used as arg; Can have default value or be a variadic
	load [IDEN, "void"]
endSignature [] // Picks the top of stack as return type, generates and pushes the signature in stack
beginFunc [] // Picks the top of stack as signature and loads it (so args are available and return type can be checked)
	load [IDEN "nil"] // struct instantiation value
	load [IDEN, "T"]
	load [IDEN, "Data"]
	call [false, 1] // This stores the actual struct (after resolving template) to stack
	createObj [1] // [ArgCount]; Instantiates a struct and loads in stack
	ret [TRUE] // [HasArg]; If has arg, it is taken from stack
endFunc [] // Now the function definition (value) is stored in stack, with the signature as type
createVar ["init", false, false, true, false, false]

// Generate an attribute list here for an instruction
// For example, for code:
//   let global const p = 10;
// will have this at bottom (of IR file(?)):
//   attributeList [<createVar Instr Id>, "global", "const"]