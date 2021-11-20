let puts = extern[puts, "<stdio.h>"] fn(data: *const i8): i32;
let strlen = extern[strlen, "<string.h>"] fn(data: *const i8): i32;
let _malloc = extern[malloc, "<stdlib.h>"] fn(size: u64): *void;
let _realloc = extern[realloc, "<stdlib.h>"] fn(data: *void, newsz: u64): *void;
let _free = extern[free, "<stdlib.h>"] fn(data: *void);

let malloc = fn(comptime T: type, count: u64): *T {
        let comptime sz = @sizeOf(T);
        return @as(@ptr(T), _malloc(sz * count));
};

let realloc = fn(comptime T: type, data: *T, count: u64): *T {
        return @as(@ptr(T), _realloc(@as(@ptr(void), data), count));
};

let free = fn(comptime T: type, data: *T) {
        _free(@as(@ptr(void), data));
};