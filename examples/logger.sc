let logger = @import("std/logger");

let main = fn(): i32 {
	logger.init(true);
	defer logger.deinit();
	logger.addTargetByName("example.log", false);
	logger.setLevel(5);

	logger.fatal("showing fatal log");
	logger.warn("showing warn log");
	logger.info("showing info log");
	logger.debug("showing debug log");
	logger.trace("showing trace log");
	return 0;
};