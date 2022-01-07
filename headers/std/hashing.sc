let cStr = fn(key: *const i8, count: u64): u64 {
	let h: u64 = 2166136261;
	while count >= 8 {
		h = (h ^ ((((*@as(@ptr(u32), key)) << 5) | ((*@as(@ptr(u32), key)) >> 27)) ^ *@as(@ptr(u32), @as(u64, key) + 4))) * 709607;
		count -= 8;
		key = @as(@ptr(i8), @as(u64, key) + 8);
	}
	if count & 4 {
		h = (h ^ *@as(@ptr(u16), key)) * 709607;
		key = @as(@ptr(i8), @as(u64, key) + 2);
		h = (h ^ *@as(@ptr(u16), key)) * 709607;
		key = @as(@ptr(i8), @as(u64, key) + 2);
	}
	if count & 2 {
		h = (h ^ *@as(@ptr(u16), key)) * 709607;
		key = @as(@ptr(i8), @as(u64, key) + 2);
	}
	if count & 1 {
		h = (h ^ *key) * 709607;
	}
	return h ^ (h >> 16);
};
