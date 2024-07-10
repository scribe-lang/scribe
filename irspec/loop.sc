let len = v.len();
let sum = 0;
for let i = 0; i < len; ++i {
	sum += v[i];
}

// IR:

load [IDEN, "v"]
attribute ["len"]
call [0, false, false, true] // [ArgCount, IsIntrinsic, IsStructInstantiate, IsMemberCall]
createVar ["len", false, false, true, false, false] // [Name: str, In: bool, Type: bool, Val: bool, Comptime: bool, Variadic: bool]
load [INT, 0]
createVar ["sum", false, false, true, false, false]

beginLoop [false, "inittag.1", "condtag.1", "incrtag.1", "blktag.1"] // [IsInlined, InitInstr, CondInstr, IncrInstr, BlockInstr]
tag ["inittag.1"]
	// init
	load [INT, 0]
	createVar ["i", false, false, true, false, false]
tag ["condtag.1"]
	// condition
	load [IDEN, "len"]
	load [IDEN, "i"]
	attribute ["<"]
	call [1, false, false, true]
	jmpFalse [<endBlock instr>]
tag ["blktag.1"]
	// block
	load [IDEN, "i"]
	load [IDEN, "v"]
	attribute ["[]"]
	call [1, false, false, true]
	load [IDEN, sum]
	attribute ["+="]
	call [1, false, false, true]
	unload [1] // [UnloadCount]
tag ["incrtag.1"]
	// increment
	load [IDEN, "i"]
	attribute ["++x"]
	call [0, false, false, true]
	unload [1]
	jmp [<condition begin instr>]
endLoop []