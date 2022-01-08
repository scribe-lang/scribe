/*
	MIT License

	Copyright (c) 2022 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#include "Utils.hpp"

namespace sc
{
Vector<StringRef> stringDelim(StringRef str, StringRef delim)
{
	Vector<StringRef> res;

	size_t start = 0;
	size_t end   = str.find(delim);
	while(end != String::npos) {
		res.emplace_back(str.substr(start, end - start));
		start = end + delim.length();
		end   = str.find(delim, start);
	}
	res.emplace_back(str.substr(start, end));

	for(auto &s : res) {
		while(!s.empty() && s.front() == ' ') s = s.substr(0, 1);
		while(!s.empty() && s.back() == ' ') s = s.substr(s.size() - 1);
	}

	return res;
}

String getRawString(StringRef data)
{
	String res;
	for(auto &e : data) {
		if(e == '\t') {
			res.push_back('\\');
			res.push_back('t');
		} else if(e == '\n') {
			res.push_back('\\');
			res.push_back('n');
		} else {
			res.push_back(e);
		}
	}
	return res;
}
} // namespace sc