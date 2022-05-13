/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef ARGS_HPP
#define ARGS_HPP

#include "Core.hpp"

namespace sc
{
namespace args
{
class ArgInfo
{
	StringRef shrt, lng; // short and long names
	StringRef val;	     // value for argument
	StringRef help;	     // help string for argument
	bool reqd;	     // required argument(?)
	bool val_reqd;	     // value required for argument(?)

public:
	ArgInfo();

	inline ArgInfo &set_short(StringRef name)
	{
		shrt = name;
		return *this;
	}
	inline ArgInfo &set_long(StringRef name)
	{
		lng = name;
		return *this;
	}
	inline ArgInfo &set_help(StringRef val)
	{
		help = val;
		return *this;
	}
	inline ArgInfo &set_reqd(bool req)
	{
		reqd = req;
		return *this;
	}
	inline ArgInfo &set_val_reqd(bool req)
	{
		val_reqd = req;
		return *this;
	}

	friend class ArgParser;
};

class ArgParser
{
	Vector<StringRef> argv;
	Map<StringRef, ArgInfo> arg_defs; // before parsing
	Map<StringRef, StringRef> opts;	  // after parsing
	Vector<StringRef> args;		  // non option arguments, after parsing

public:
	ArgParser(const int &argc, const char **argv);

	ArgInfo &add(StringRef argname);
	void parse();
	void print_help(FILE *file);
	inline void clear()
	{
		arg_defs.clear();
		opts.clear();
		args.clear();
	}

	// retrieve info
	inline bool has(StringRef argname)
	{
		return opts.find(argname) != opts.end();
	}
	StringRef val(StringRef argname)
	{
		if(has(argname)) return opts[argname];
		return "";
	}
	inline StringRef get(const size_t &idx)
	{
		return idx >= args.size() ? "" : args[idx];
	}
};
} // namespace args
} // namespace sc

#endif // ARGS_HPP