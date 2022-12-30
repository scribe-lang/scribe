#pragma once

namespace sc
{

enum InstrTy
{
	ENTRYPOINT,   // define the entry point of the program
	CONSTDATA,    // declare a const data (strings, as of now)
	CREATEVAR,    // declare/define a new variable using the available information
	CREATESTRUCT, // define a new struct
	CREATEFN,     // define a new function
	CALL,	      // call a function (can be intrinsic)
	BASICBLOCK,   // define a basic block (equivalent to LLVM's basic block)
	INITSTRUCT,   // create a struct instance
	RETURN,	      // return data from function
	DOT,	      // get a field from an aggregate
	LOOP,	      // create a loop
	JMP,	      // jump unconditionally
	JMPTRUE,      // jump if condition is true
	JMPFALSE,     // jump if condition is false
	BINOP,	      // binary operation (+, -, etc.)
	UNOP,	      // unary operation (++, --, etc.)
		      /* Utility */
	PTR,	      // add pointer to a type
	REF,	      // add ref to a type
	CONST,	      // add const to a type
		      /* Eh */
	INVALID,      // invalid operation
};

} // namespace sc