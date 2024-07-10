let Platform = @enum(i8, .Windows, .Linux, .MacOS);

let x: Platform;
let y = Platform.Windows;

// IR:

load [IDEN, "i8"]
load [STR, "Windows"]
load [STR, "Linux"]
load [STR, "MacOS"]
load [IDEN, "enum"]
call [4, true, false, false] // [ArgCount, IsIntrinsic, IsStructInstantiate, IsMemberCall]
createVar ["Platform", false, false, true, false, false] // [Name: str, In: bool, Type: bool, Val: bool, Comptime: bool, Variadic: bool]

load [IDEN, "Platform"]
createVar ["x", false, true, false, false, false]

load [IDEN, "Platform"]
attribute ["Windows"] // [AttrName]; Finds AttrName in the element at the top of stack
createVar ["y", false, false, true, false, false]