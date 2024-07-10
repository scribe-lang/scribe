inline if (a == b && c == d) || e == f {
let p = 1;
} elif g == h {
let p = 2;
} else {
let p = 3;
}

// IR:

beginConditional [true, "condtag.1", "condtag.2", "condtag.3"] // [IsInlined, ConditionalBlocks...]
tag ["condtag.1"]
	load [IDEN, "b"] // 0
	load [IDEN, "a"] // 1
	attribute ["=="] // 2
	call [1, false, false, true] // 3 // [ArgCount, IsIntrinsic, IsStructInstantiate, IsMemberCall]
	jmpFalse [10] // 4
	load [IDEN, "d"] // 5
	load [IDEN, "c"] // 6
	attribute ["=="] // 7
	call [1, false, false, true] // 8
	jmpTrue [15] // 9
	load [IDEN, "f"] // 10
	load [IDEN, "e"] // 11
	attribute ["=="] // 12
	call [1, false, false, true] // 13
	jmpFalse [19] // 14
	beginBlock [true] // 15 // [IsInlined]
		load [INT, 1] // 16
		createVar ["p", false, false, true, false, false] // 17 // [Name: str, In: bool, Type: bool, Val: bool, Comptime: bool, Variadic: bool]
	endBlock [] // 18
tag ["condtag.2"]
	load [IDEN, "h"] // 19
	load [IDEN, "g"] // 20
	attribute ["=="] // 21
	call [1, false, false, true] // 22
	jmpFalse [39] // 23
	beginBlock [true] // 24
		load [INT, 2] // 25
		createVar ["p", false, false, true, false, false] // 26
	endBlock [] // 27
tag ["condtag.3"]
	beginBlock [true] // 28
		load [INT, 3] // 29
		createVar ["p", false, false, true, false, false] // 30
	endBlock [] // 31
endConditional []






load [IDEN, "b"] // 0
load [IDEN, "a"] // 1
attribute ["=="] // 2
call [1, false, false, true] // 3 // [ArgCount, IsIntrinsic, IsStructInstantiate, IsMemberCall]
jmpFalse [10] // 4
load [IDEN, "d"] // 5
load [IDEN, "c"] // 6
attribute ["=="] // 7
call [1, false, false, true] // 8
jmpTrue [15] // 9
load [IDEN, "f"] // 10
load [IDEN, "e"] // 11
attribute ["=="] // 12
call [1, false, false, true] // 13
jmpFalse [19] // 14
beginBlock [true] // 15 // [IsInlined]
	load [INT, 1] // 16
	createVar ["p", false, false, true, false, false] // 17 // [Name: str, In: bool, Type: bool, Val: bool, Comptime: bool, Variadic: bool]
endBlock [] // 18
load [IDEN, "h"] // 19
load [IDEN, "g"] // 20
attribute ["=="] // 21
call [1, false, false, true] // 22
jmpFalse [39] // 23
beginBlock [true] // 24
	load [INT, 2] // 25
	createVar ["p", false, false, true, false, false] // 26
endBlock [] // 27
beginBlock [true] // 28
	load [INT, 3] // 29
	createVar ["p", false, false, true, false, false] // 30
endBlock [] // 31