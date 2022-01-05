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
std::vector<std::string> stringDelim(const std::string &str, const std::string &delim)
{
	std::vector<std::string> res;

	size_t start = 0;
	size_t end   = str.find(delim);
	while(end != std::string::npos) {
		res.push_back(str.substr(start, end - start));
		start = end + delim.length();
		end   = str.find(delim, start);
	}
	res.push_back(str.substr(start, end));

	for(auto &s : res) {
		while(!s.empty() && s.front() == ' ') s.erase(s.begin());
		while(!s.empty() && s.back() == ' ') s.pop_back();
	}

	return res;
}

std::string getRawString(const std::string &data)
{
	std::string res;
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