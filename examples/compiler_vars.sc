let comptime buildDate = @getCompilerVar("BUILD_DATE"); // returns StringRef (or perhaps *const i8?)
let comptime currentOS = @getCompilerVar("OSId"); // returns int