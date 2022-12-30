let io = @import("std/io");

let Data = struct<T> {
	list: *T;
};

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

// note: change all "subs" instructions to: binop "__subscr__"
// all createVar must be at the top of the body of the function

// inserted at the end
entryPoint %81

%1 = constData "std/io" // -> ConstDataValInstr
%2 = constData "import" // -> ConstDataValInstr
%3 = call %2, true, %1 // call(fn: FnValInstr, intrin: bool, args: ...Instr) -> RetValInstr
%4 = createVar "io" nil, nil, %3, false // createVar(name: string, in: TypeValInstr/nil, type: TypeValInstr/nil, val: Instr/nil, comptime: bool) -> VarValInstr

%5 = createStruct T { // createStruct(templates: ...string) { definition } -> TypeValInstr
	%6 = getVar "T" // getVar(name: string) -> VarValInstr
	%7 = @ptr(%6) // -> TypeValInstr
	%8 = createVar "list", %7, nil, false
}
%9 = createVar "Data", nil, nil, %5, false

// args must be executed first since they may contain templates which could be used by return type
%20 = createFn %14, %11 { // createFn(ret: TypeValInstr, args: ...VarValInstr) -> TypeValInstr
args:
	%10 = getVar "type"
	%11 = createVar "T", nil, %10, nil, true
returns:
	%12 = getVar "T"
	%13 = getVar "Data"
	%14 = call %13, false, %12
body:
	%15 = getVar "T"
	%16 = getVar "Data"
	%17 = call %16, false, %15
	%18 = getVar "nil"
	%19 = initStruct %17, %18 // initStruct(struct: TypeValInstr, args: ...VarValInstr)
	ret %19
}
%21 = createVar "init", nil, %15, false

%34 = createFn, %29, %24, %26 {
args:
	%22 = getVar "Data"
	%23 = @ref(%22) // -> TypeValInstr
	%24 = createVar "self", nil, %23, nil, false
	%25 = getVar "u64"
	%26 = createVar "idx", nil, %25, nil, false
returns:
	%27 = getVar "self"
	%28 = dot %27, "T" // dot(aggregate: ValInstr, field: string) -> ValInstr
	%29 = @ref(%28)
body:
	%30 = getVar "self"
	%31 = dot %30, "list"
	%32 = getVar "idx"
	%33 = binop "__subscr__", %31, %32 // binop(name: string, lhs: ValInstr, rhs: ValInstr) -> ValInstr
	ret %33
}
%35 = getVar "Data"
%36 = createVar "__subscr__", %35, nil, %34, false

%45 = createFn %43, %40 {
args:
	%37 = getVar "Data"
	%38 = @const(%37)
	%39 = @ref(%38)
	%40 = createVar "self", nil, %39, nil, false
returns:
	%41 = getVar "i8"
	%42 = @const(%41)
	%43 = %ptr(%42)
body:
	%44 = constData "Hi"
	ret %44
}
%46 = getVar "Data"
%47 = @const(%46)
%48 = createVar "str", %47, nil, %45, false

%80 = createFn %49 {
args:
returns:
	%49 = getVar "i32"
body:
	%50 = getVar "init"
	%51 = getVar "i32"
	%52 = call %50, false, %51
	%53 = createVar "data", nil, nil, %52, false

	%54 = constData 0
	%55 = createVar "d", nil, nil, %54, false

	// loop(begin: Block/nil, cond: Block/nil, body: Block/nil, incr: Block/nil)
	loop loop_begin.1, loop_cond.1, loop_body.1, loop_incr.1
loop_begin.1:
	%56 = constData 0
	%57 = createVar "i", nil, nil, %56, false
loop_cond.1:
	%58 = constData 10
	%59 = getVar "i"
	%60 = binop "__lt__", %59, %58
	jmp_false %60, loop_end.1
loop_body.1:
	%61 = constData 5
	%62 = getVar "i"
	%63 = binop "__eq__", %62, %61
	jmp_false %63, cond_end.1
	jmp loop_incr.1 // equivalent of continue
cond_end.1:
	%64 = getVar "i"
	%65 = getVar "data"
	%66 = binop "__subscr__", %65, %64
	%67 = getVar "d"
	%68 = binop "__add_assn__", %67, %66
loop_incr.1:
	%69 = getVar "i"
	%70 = unop "__pre_incr__", %69 // unop(name: string, operand: ValInstr) -> ValInstr
	jmp loop_cond.1
loop_end.1:

	%75 = getVar "io"
	%76 = dot %75, "println"
	%77 = getVar "d"
	%78 = call %76, false, %77
	%79 = constData 0
	ret %79
}
%81 = createVar "main", nil, nil, %80, false