/////////////////////////////////////////
// Struct definition
/////////////////////////////////////////

let Vec = struct(comptime T: type, data: *T, managed: i1, count: usize, capacity: usize);

// IR:

beginSignature [5, STRUCT, false] // [ArgCount (template inclusive), Func/Struct/Union, IsExtern]
	// VM must do some *magic* to enable use of "Self" as a type within the struct
	load [IDEN, "type"]
	createVar ["T", false, true, false, true, false] // [Name: str, In: bool, Type: bool, Val: bool, Comptime: bool, Variadic: bool]
	load [IDEN, "T"]
	load [IDEN, "typePtr"]
	call [1, true, false, false] // [ArgCount, IsIntrinsic, IsStructInstantiate, IsMemberCall]
	createVar ["data", false, true, false, false, false]
	load [IDEN, "i1"]
	createVar ["managed", false, true, false, false, false]
	load [IDEN "usize"]
	createVar ["count", false, true, false, false, false]
	load [IDEN "usize"]
	createVar ["capacity", false, true, false, false, false]
endSignature [] // Pushes the signature in the stack. If it's a func, top of stack is taken as return type before pushing the signature.
createVar ["Vec", false, false, true, false, false]

/////////////////////////////////////////
// Struct instantiation
/////////////////////////////////////////

let i32Vec = Vec(i32){nil, true, 0, 0};

// IR:

load [INT, 0]
load [INT, 0]
load [TRUE]
load [NIL]
load [IDEN, "i32"]
load [IDEN, "Vec"]
call [1, false, false, false] // create i32 vec struct
call [4, false, true, false] // create i32 vec object
createVar ["i32Vec", false, false, true, false, false]
