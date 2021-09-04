/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "../include/Args.hpp"

#include <cstdio>
#include <stdexcept>

#include "../include/Config.hpp"

namespace sc
{
namespace args
{
ArgInfo::ArgInfo() : reqd(false), val_reqd(false) {}

ArgParser::ArgParser(const int &argc, const char **argv)
{
	for(int i = 0; i < argc; ++i) this->argv.push_back(argv[i]);

	arg_defs["help"].set_long("help").set_short("h").set_help(
	"prints help information for program");
}

ArgInfo &ArgParser::add(const std::string &argname)
{
	arg_defs[argname].lng = argname;
	return arg_defs[argname];
}

void ArgParser::parse()
{
	std::string expect_key;
	bool expect_val = false;
	for(size_t i = 0; i < argv.size(); ++i) {
		std::string arg = argv[i];
		if(expect_val) {
			opts[expect_key] = arg;
			expect_val	 = false;
			continue;
		}
		if(arg.rfind("--", 0) == 0) {
			arg.erase(0, 2);
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
			arg.erase(0, 1);
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
			throw std::runtime_error("Required argument: " + a.first +
						 " was not provided");
		}
	}
	return;
}

void ArgParser::print_help(FILE *file)
{
	fprintf(file, "%s compiler %d.%d.%d (language %d.%d.%d)\n", PROJECT_NAME, SCRIBE_MAJOR,
		SCRIBE_MINOR, SCRIBE_PATCH, COMPILER_MAJOR, COMPILER_MINOR, COMPILER_PATCH);

	std::string reqd_args;
	for(auto &arg : arg_defs) {
		if(arg.second.reqd) {
			reqd_args += "[" + arg.first + "] ";
		}
	}
	fprintf(file, "usage: %s%s <args>\n\n", argv[0].c_str(), reqd_args.c_str());
	for(auto &arg : arg_defs) {
		if(!arg.second.shrt.empty()) {
			fprintf(file, "-%s, --%s\t\t%s\n", arg.second.shrt.c_str(),
				arg.second.lng.c_str(), arg.second.help.c_str());
		} else {
			fprintf(file, "--%s\t\t%s\n", arg.second.lng.c_str(),
				arg.second.help.c_str());
		}
	}
}
} // namespace args
} // namespace sc