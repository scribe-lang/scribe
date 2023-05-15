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

#include "../include/FS.hpp"

#include "../include/Env.hpp"

namespace sc
{
namespace fs
{
static int total_lines = 0;

int getLastTotalLines() { return total_lines; }

bool read(const String &file, String &data)
{
	FILE *fp;
	char buf[256];

	fp = fopen(file.c_str(), "r");
	if(fp == NULL) {
		fprintf(stderr, "Error: failed to open source file: %s\n", file.c_str());
		return false;
	}

	while(fgets(buf, sizeof(buf), fp) != NULL) data += buf;

	for(auto c : data) {
		if(c == '\n') ++total_lines;
	}

	fclose(fp);

	if(data.empty()) {
		fprintf(stderr, "Error: encountered empty file: %s\n", file.c_str());
		return false;
	}

	return true;
}

static String _home()
{
	String home;
#if defined(OS_WINDOWS)
	home = env::get("USERPROFILE");
	if(home.empty()) home = env::get("HOMEDRIVE") + env::get("HOMEPATH");
#else
	home = env::get("HOME");
#endif
	return home;
}

String home()
{
	static String __home = _home();
	return __home;
}
} // namespace fs
} // namespace sc