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

#ifndef ARGS_HPP
#define ARGS_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace sc
{
namespace args
{
class ArgInfo
{
	std::string shrt, lng; // short and long names
	std::string val;       // value for argument
	std::string help;      // help string for argument
	bool reqd;	       // required argument(?)
	bool val_reqd;	       // value required for argument(?)

public:
	ArgInfo();

	inline ArgInfo &set_short(const std::string &name)
	{
		shrt = name;
		return *this;
	}
	inline ArgInfo &set_long(const std::string &name)
	{
		lng = name;
		return *this;
	}
	inline ArgInfo &set_help(const std::string &val)
	{
		help = val;
		return *this;
	}
	inline ArgInfo &set_reqd(const bool &req)
	{
		reqd = req;
		return *this;
	}
	inline ArgInfo &set_val_reqd(const bool &req)
	{
		val_reqd = req;
		return *this;
	}

	friend class ArgParser;
};

class ArgParser
{
	std::vector<std::string> argv;
	std::unordered_map<std::string, ArgInfo> arg_defs; // before parsing
	std::unordered_map<std::string, std::string> opts; // after parsing
	std::vector<std::string> args;			   // non option arguments, after parsing

public:
	ArgParser(const int &argc, const char **argv);

	ArgInfo &add(const std::string &argname);
	void parse();
	void print_help(FILE *file);
	inline void clear()
	{
		arg_defs.clear();
		opts.clear();
		args.clear();
	}

	// retrieve info
	inline bool has(const std::string &argname)
	{
		return opts.find(argname) != opts.end();
	}
	std::string val(const std::string &argname)
	{
		return has(argname) ? opts[argname] : "";
	}
	inline std::string get(const size_t &idx)
	{
		return idx >= args.size() ? "" : args[idx];
	}
};
} // namespace args
} // namespace sc

#endif // ARGS_HPP