// Here, struct() syntax is similar to a function declaration syntax
let Vec = struct(comptime T: type, data: *T);
// Enum creation can be done simply as a function call instead of
// requiring a dedicated syntax.
let Platform = @enum(i8, .Windows, .Linux, .MacOS);
let plat: Platform; // => let plat: i8;

let SomeStruct = struct(data: *i32);

let DT_REG = @externVar(u8, .DT_REG);