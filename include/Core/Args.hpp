#pragma once

#include "Core.hpp"

namespace sc::args
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

	inline ArgInfo &setShort(StringRef name)
	{
		shrt = name;
		return *this;
	}
	inline ArgInfo &setLong(StringRef name)
	{
		lng = name;
		return *this;
	}
	inline ArgInfo &setHelp(StringRef val)
	{
		help = val;
		return *this;
	}
	inline ArgInfo &setReqd(bool req)
	{
		reqd = req;
		return *this;
	}
	inline ArgInfo &setValReqd(bool req)
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
	ArgParser(int argc, const char **argv);

	ArgInfo &add(StringRef argname);
	void parse();
	void printHelp(OStream &os);
	inline void clear()
	{
		arg_defs.clear();
		opts.clear();
		args.clear();
	}

	// retrieve info
	inline bool has(StringRef argname) { return opts.find(argname) != opts.end(); }
	StringRef val(StringRef argname)
	{
		if(has(argname)) return opts[argname];
		return "";
	}
	inline StringRef get(size_t idx) { return idx >= args.size() ? "" : args[idx]; }
	inline const Vector<StringRef> &getArgv() const { return argv; }
};
} // namespace sc::args
