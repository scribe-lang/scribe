let v = vec.new(i32, true);
v.insert(2, 25) or e {
    io.println("failed to insert data: ", e.msg(), e.trace());
};