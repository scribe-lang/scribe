@useCLib("<stdlib.h>");

let malloc = extern fn(byteCount: usize): *void;
let someStruct = extern struct(data: *void);

// IR:

load [STR, "<stdlib.h>"]
load [IDEN, "useCLib"]
call [1, true, false, false] // [ArgCount, IsIntrinsic, IsStructInstantiate, IsMemberCall]
unload [1] // [UnloadCount]

beginSignature [1, FUNC, true] // [ArgCount (template inclusive), Func/Struct/Union, IsExtern]
	load [IDEN, "usize"]
	createVar ["byteCount", false, true, false, false, false] // [Name: str, In: bool, Type: bool, Val: bool, Comptime: bool, Variadic: bool]
	load [IDEN, "void"]
	load [IDEN, "typePtr"]
	call [1, true, false, false]
endSignature [] // Pushes the signature in the stack. If it's a func, top of stack is taken as return type before pushing the signature.
createVar ["malloc", false, false, true, false, false] // [Name: str, In: bool, Type: bool, Val: bool, Comptime: bool, Variadic: bool]

beginSignature [1, STRUCT, true]
	load [IDEN, "void"]
	load [IDEN, "typePtr"]
	call [1, true, false, false]
	createVar ["data", false, true, false, false, false]
endSignature []
createVar ["someStruct", false, false, true, false, false]
