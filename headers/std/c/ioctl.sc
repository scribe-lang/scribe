let unistd = @import("std/c/unistd");

let winsize = extern[struct winsize, "<sys/ioctl.h>"] struct {
	ws_row: u16;
	ws_col: u16;
	ws_xpixel: u16;
	ws_ypixel: u16;
};

let ioctl = extern[ioctl, "<sys/ioctl.h>"] fn(fd: i32, cmd: i32, args: ...any);

let TCGETS: i32 = extern[TCGETS, "<sys/ioctl.h>"];
let TCSETS: i32 = extern[TCSETS, "<sys/ioctl.h>"];
let TCSETSW: i32 = extern[TCSETSW, "<sys/ioctl.h>"];
let TCSETSF: i32 = extern[TCSETSF, "<sys/ioctl.h>"];
let TCGETA: i32 = extern[TCGETA, "<sys/ioctl.h>"];
let TCSETA: i32 = extern[TCSETA, "<sys/ioctl.h>"];
let TCSETAW: i32 = extern[TCSETAW, "<sys/ioctl.h>"];
let TCSETAF: i32 = extern[TCSETAF, "<sys/ioctl.h>"];
let TCSBRK: i32 = extern[TCSBRK, "<sys/ioctl.h>"];
let TCXONC: i32 = extern[TCXONC, "<sys/ioctl.h>"];
let TCFLSH: i32 = extern[TCFLSH, "<sys/ioctl.h>"];
let TIOCEXCL: i32 = extern[TIOCEXCL, "<sys/ioctl.h>"];
let TIOCNXCL: i32 = extern[TIOCNXCL, "<sys/ioctl.h>"];
let TIOCSCTTY: i32 = extern[TIOCSCTTY, "<sys/ioctl.h>"];
let TIOCGPGRP: i32 = extern[TIOCGPGRP, "<sys/ioctl.h>"];
let TIOCSPGRP: i32 = extern[TIOCSPGRP, "<sys/ioctl.h>"];
let TIOCOUTQ: i32 = extern[TIOCOUTQ, "<sys/ioctl.h>"];
let TIOCSTI: i32 = extern[TIOCSTI, "<sys/ioctl.h>"];
let TIOCGWINSZ: i32 = extern[TIOCGWINSZ, "<sys/ioctl.h>"];
let TIOCSWINSZ: i32 = extern[TIOCSWINSZ, "<sys/ioctl.h>"];
let TIOCMGET: i32 = extern[TIOCMGET, "<sys/ioctl.h>"];
let TIOCMBIS: i32 = extern[TIOCMBIS, "<sys/ioctl.h>"];
let TIOCMBIC: i32 = extern[TIOCMBIC, "<sys/ioctl.h>"];
let TIOCMSET: i32 = extern[TIOCMSET, "<sys/ioctl.h>"];
let TIOCGSOFTCAR: i32 = extern[TIOCGSOFTCAR, "<sys/ioctl.h>"];
let TIOCSSOFTCAR: i32 = extern[TIOCSSOFTCAR, "<sys/ioctl.h>"];
let FIONREAD: i32 = extern[FIONREAD, "<sys/ioctl.h>"];
let TIOCINQ: i32 = extern[TIOCINQ, "<sys/ioctl.h>"];
let TIOCLINUX: i32 = extern[TIOCLINUX, "<sys/ioctl.h>"];
let TIOCCONS: i32 = extern[TIOCCONS, "<sys/ioctl.h>"];
let TIOCGSERIAL: i32 = extern[TIOCGSERIAL, "<sys/ioctl.h>"];
let TIOCSSERIAL: i32 = extern[TIOCSSERIAL, "<sys/ioctl.h>"];
let TIOCPKT: i32 = extern[TIOCPKT, "<sys/ioctl.h>"];
let FIONBIO: i32 = extern[FIONBIO, "<sys/ioctl.h>"];
let TIOCNOTTY: i32 = extern[TIOCNOTTY, "<sys/ioctl.h>"];
let TIOCSETD: i32 = extern[TIOCSETD, "<sys/ioctl.h>"];
let TIOCGETD: i32 = extern[TIOCGETD, "<sys/ioctl.h>"];
let TCSBRKP: i32 = extern[TCSBRKP, "<sys/ioctl.h>"];
let TIOCSBRK: i32 = extern[TIOCSBRK, "<sys/ioctl.h>"];
let TIOCCBRK: i32 = extern[TIOCCBRK, "<sys/ioctl.h>"];
let TIOCGSID: i32 = extern[TIOCGSID, "<sys/ioctl.h>"];
let TCGETS2: i32 = extern[TCGETS2, "<sys/ioctl.h>"];
let TCSETS2: i32 = extern[TCSETS2, "<sys/ioctl.h>"];
let TCSETSW2: i32 = extern[TCSETSW2, "<sys/ioctl.h>"];
let TCSETSF2: i32 = extern[TCSETSF2, "<sys/ioctl.h>"];
let TIOCGRS485: i32 = extern[TIOCGRS485, "<sys/ioctl.h>"];
let TIOCGPTN: i32 = extern[TIOCGPTN, "<sys/ioctl.h>"];
let TIOCSPTLCK: i32 = extern[TIOCSPTLCK, "<sys/ioctl.h>"];
let TIOCGDEV: i32 = extern[TIOCGDEV, "<sys/ioctl.h>"];
let TCGETX: i32 = extern[TCGETX, "<sys/ioctl.h>"];
let TCSETX: i32 = extern[TCSETX, "<sys/ioctl.h>"];
let TCSETXF: i32 = extern[TCSETXF, "<sys/ioctl.h>"];
let TCSETXW: i32 = extern[TCSETXW, "<sys/ioctl.h>"];
let TIOCSIG: i32 = extern[TIOCSIG, "<sys/ioctl.h>"];
let TIOCVHANGUP: i32 = extern[TIOCVHANGUP, "<sys/ioctl.h>"];
let TIOCGPKT: i32 = extern[TIOCGPKT, "<sys/ioctl.h>"];
let TIOCGPTLCK: i32 = extern[TIOCGPTLCK, "<sys/ioctl.h>"];
let TIOCGEXCL: i32 = extern[TIOCGEXCL, "<sys/ioctl.h>"];
let TIOCGPTPEER: i32 = extern[TIOCGPTPEER, "<sys/ioctl.h>"];
let TIOCGISO7816: i32 = extern[TIOCGISO7816, "<sys/ioctl.h>"];
let TIOCSISO7816: i32 = extern[TIOCSISO7816, "<sys/ioctl.h>"];

let FIONCLEX: i32 = extern[FIONCLEX, "<sys/ioctl.h>"];
let FIOCLEX: i32 = extern[FIOCLEX, "<sys/ioctl.h>"];
let FIOASYNC: i32 = extern[FIOASYNC, "<sys/ioctl.h>"];
let TIOCSERCONFIG: i32 = extern[TIOCSERCONFIG, "<sys/ioctl.h>"];
let TIOCSERGWILD: i32 = extern[TIOCSERGWILD, "<sys/ioctl.h>"];
let TIOCSERSWILD: i32 = extern[TIOCSERSWILD, "<sys/ioctl.h>"];
let TIOCGLCKTRMIOS: i32 = extern[TIOCGLCKTRMIOS, "<sys/ioctl.h>"];
let TIOCSLCKTRMIOS: i32 = extern[TIOCSLCKTRMIOS, "<sys/ioctl.h>"];
let TIOCSERGSTRUCT: i32 = extern[TIOCSERGSTRUCT, "<sys/ioctl.h>"];
let TIOCSERGETLSR: i32 = extern[TIOCSERGETLSR, "<sys/ioctl.h>"];
let TIOCSERGETMULTI: i32 = extern[TIOCSERGETMULTI, "<sys/ioctl.h>"];
let TIOCSERSETMULTI: i32 = extern[TIOCSERSETMULTI, "<sys/ioctl.h>"];

let TIOCMIWAIT: i32 = extern[TIOCMIWAIT, "<sys/ioctl.h>"];
let TIOCGICOUNT: i32 = extern[TIOCGICOUNT, "<sys/ioctl.h>"];

let FIOQSIZE: i32 = extern[FIOQSIZE, "<sys/ioctl.h>"];

/* Used for packet mode */
let TIOCPKT_DATA: i32 = extern[TIOCPKT_DATA, "<sys/ioctl.h>"];
let TIOCPKT_FLUSHREAD: i32 = extern[TIOCPKT_FLUSHREAD, "<sys/ioctl.h>"];
let TIOCPKT_FLUSHWRITE: i32 = extern[TIOCPKT_FLUSHWRITE, "<sys/ioctl.h>"];
let TIOCPKT_STOP: i32 = extern[TIOCPKT_STOP, "<sys/ioctl.h>"];
let TIOCPKT_START: i32 = extern[TIOCPKT_START, "<sys/ioctl.h>"];
let TIOCPKT_NOSTOP: i32 = extern[TIOCPKT_NOSTOP, "<sys/ioctl.h>"];
let TIOCPKT_DOSTOP: i32 = extern[TIOCPKT_DOSTOP, "<sys/ioctl.h>"];
let TIOCPKT_IOCTL: i32 = extern[TIOCPKT_IOCTL, "<sys/ioctl.h>"];

let TIOCSER_TEMT: i32 = extern[TIOCSER_TEMT, "<sys/ioctl.h>"];