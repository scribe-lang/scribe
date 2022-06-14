// Spirally traverse/display a 2D array in clockwise manner

let io = @import("std/io");

let dispSpiral = fn(a: &const @array(i32, 4, 4), hx: i32, hy: i32) {
	let i = 0;
	let j = 0;
	let iinc = 1;
	let jinc = 1;
	let lx = 1;
	let ly = 0;
	let iorj = true; // false = i, true = j
	while lx < hx || ly < hy {
		if iorj && ((jinc < 0 && j == ly) || (jinc > 0 && j == hy - 1)) {
			if jinc < 0 {
				++ly;
			} elif jinc > 0 {
				--hy;
			}
			jinc *= -1;
			iorj = false;
		} elif !iorj && ((iinc < 0 && i == lx) || (iinc > 0 && i == hx - 1)) {
			if iinc < 0 {
				++lx;
			} elif iinc > 0 {
				--hx;
			}
			iinc *= -1;
			iorj = true;
		}
		io.println(a[i][j], " ", i, " ", j);
		if iorj { j += jinc; }
		else { i += iinc; }
	}
};

let main = fn(): i32 {
	let a = @array(i32, 4, 4);
	let tot = 1;
	for let i = 0; i < 4; ++i {
		for let j = 0; j < 4; ++j {
			a[i][j] = tot++;
		}
	}
	for let i = 0; i < 4; ++i {
		for let j = 0; j < 4; ++j {
			io.print(a[i][j], " ");
		}
		io.println();
	}
	io.println();
	dispSpiral(a, 4, 4);
	return 0;
};