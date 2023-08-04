#include "Args.hpp"

#include "Config.hpp"

namespace sc
{
namespace args
{
ArgInfo::ArgInfo() : reqd(false), val_reqd(false) {}

ArgParser::ArgParser(int argc, const char **argv)
{
	for(int i = 0; i < argc; ++i) this->argv.push_back(argv[i]);

	arg_defs["help"].setLong("help").setShort("h").setHelp(
	"prints help information for program");
}

ArgInfo &ArgParser::add(StringRef argname)
{
	arg_defs[argname].lng = argname;
	return arg_defs[argname];
}

void ArgParser::parse()
{
	String expect_key;
	bool expect_val = false;
	for(size_t i = 0; i < argv.size(); ++i) {
		StringRef arg = argv[i];
		if(expect_val) {
			opts[expect_key] = arg;
			expect_val	 = false;
			continue;
		}
		if(arg.rfind("--", 0) == 0) {
			arg = arg.substr(2);
			for(auto &a : arg_defs) {
				if(a.second.lng == arg) {
					opts.insert({a.first, ""});
					if(a.second.reqd) a.second.reqd = false;
					if(a.second.val_reqd) {
						expect_key = a.first;
						expect_val = true;
					}
				}
			}
			continue;
		}
		if(arg.rfind("-", 0) == 0) {
			arg = arg.substr(1);
			for(auto &a : arg_defs) {
				if(a.second.shrt == arg) {
					opts.insert({a.first, ""});
					if(a.second.reqd) a.second.reqd = false;
					if(a.second.val_reqd) {
						expect_key = a.first;
						expect_val = true;
					}
				}
			}
			continue;
		}
		args.push_back(arg);
	}
	if(expect_val) {
		throw std::runtime_error("Expected value to be provided for argument: " +
					 expect_key);
	}
	for(auto &a : arg_defs) {
		if(a.second.reqd && opts.find(a.first) == opts.end()) {
			throw std::runtime_error("Required argument: " + String(a.first) +
						 " was not provided");
		}
	}
	return;
}

void ArgParser::printHelp(FILE *file)
{
	std::cout << PROJECT_NAME << " compiler " << COMPILER_MAJOR << "." << COMPILER_MINOR << "."
		  << COMPILER_PATCH << "\n";

	std::cout << "usage: " << argv[0];
	for(auto &arg : arg_defs) {
		if(arg.second.reqd) {
			std::cout << " [" << arg.first << "]";
		}
	}
	std::cout << " <args>\n\n";
	for(auto &arg : arg_defs) {
		if(!arg.second.shrt.empty()) {
			std::cout << "-" << arg.second.shrt << ", --" << arg.second.lng << "\t\t"
				  << arg.second.help << "\n";
		} else {
			std::cout << "--" << arg.second.lng << "\t\t" << arg.second.help << "\n";
		}
	}
}
} // namespace args
} // namespace sc