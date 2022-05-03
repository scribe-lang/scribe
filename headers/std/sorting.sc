// General sorting comparators

let c = @import("std/c");
let string = @import("std/string");

let i1Cmp = fn(a: &const i1, b: &const i1): i32 { return @as(i32, a) - @as(i32, b); };
let i8Cmp = fn(a: &const i8, b: &const i8): i32 { return @as(i32, a) - @as(i32, b); };
let i16Cmp = fn(a: &const i16, b: &const i16): i32 { return @as(i32, a) - @as(i32, b); };
let i32Cmp = fn(a: &const i32, b: &const i32): i32 { return @as(i32, a) - @as(i32, b); };
let i64Cmp = fn(a: &const i64, b: &const i64): i32 { return @as(i32, a) - @as(i32, b); };
let u8Cmp = fn(a: &const u8, b: &const u8): i32 { return @as(i32, a) - @as(i32, b); };
let u16Cmp = fn(a: &const u16, b: &const u16): i32 { return @as(i32, a) - @as(i32, b); };
let u32Cmp = fn(a: &const u32, b: &const u32): i32 { return @as(i32, a) - @as(i32, b); };
let u64Cmp = fn(a: &const u64, b: &const u64): i32 { return @as(i32, a) - @as(i32, b); };
let f32Cmp = fn(a: &const f32, b: &const f32): i32 { return @as(i32, a) - @as(i32, b); };
let f64Cmp = fn(a: &const f64, b: &const f64): i32 { return @as(i32, a) - @as(i32, b); };

let cStrCmp = fn(a: *&const i8, b: *&const i8): i32 { return c.strcmp(a, b); };
let strCmp = fn(a: &const string.String, b: &const string.String): i32 { return c.strcmp(a.cStr(), b.cStr()); };
