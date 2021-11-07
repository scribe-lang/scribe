let test = @import("./to_import");

let p = test.x;
let comptime res = test.add(p, p);