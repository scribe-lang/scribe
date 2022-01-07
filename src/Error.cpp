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

#include "Error.hpp"

#include <cstring>

#include "Parser.hpp"

namespace sc
{
ModuleLoc::ModuleLoc(Module *mod, const size_t &line, const size_t &col)
	: mod(mod), line(line), col(col)
{}

std::string ModuleLoc::getLocStr() const
{
	return std::to_string(line) + ":" + std::to_string(col);
}

void ErrMgr::set(Stmt *stmt, const char *e, ...)
{
	va_list args;
	va_start(args, e);
	set(stmt->getLoc(), e, args);
	va_end(args);
}
void ErrMgr::set(const lex::Lexeme &tok, const char *e, ...)
{
	va_list args;
	va_start(args, e);
	set(tok.getLoc(), e, args);
	va_end(args);
}
void ErrMgr::set(const ModuleLoc &loc, const char *e, ...)
{
	va_list args;
	va_start(args, e);
	set(loc, e, args);
	va_end(args);
}
void ErrMgr::set(const ModuleLoc &loc, const char *e, va_list args)
{
	locs.insert(locs.begin(), loc);
	warns.insert(warns.begin(), false);

	static char msg[4096];
	std::memset(msg, 0, 4096);
	vsprintf(msg, e, args);
	errs.insert(errs.begin(), msg);
}

// equivalent to set(), but for warnings
void ErrMgr::setw(Stmt *stmt, const char *e, ...)
{
	va_list args;
	va_start(args, e);
	setw(stmt->getLoc(), e, args);
	va_end(args);
}
void ErrMgr::setw(const lex::Lexeme &tok, const char *e, ...)
{
	va_list args;
	va_start(args, e);
	setw(tok.getLoc(), e, args);
	va_end(args);
}
void ErrMgr::setw(const ModuleLoc &loc, const char *e, ...)
{
	va_list args;
	va_start(args, e);
	setw(loc, e, args);
	va_end(args);
}
void ErrMgr::setw(const ModuleLoc &loc, const char *e, va_list args)
{
	locs.insert(locs.begin(), loc);
	warns.insert(warns.begin(), true);

	static char msg[4096];
	std::memset(msg, 0, 4096);
	vsprintf(msg, e, args);
	errs.insert(errs.begin(), msg);
}

bool ErrMgr::present()
{
	return !errs.empty();
}
void ErrMgr::show(FILE *out)
{
	while(!errs.empty()) {
		Module *mod	     = locs.back().mod;
		const size_t &line   = locs.back().line;
		const size_t &col    = locs.back().col;
		const bool &warn     = warns.back();
		const std::string &e = errs.back();

		size_t linectr = 0;
		size_t idx     = 0;
		bool found     = false;

		const std::string &data	    = mod->getCode();
		const std::string &filename = mod->getPath();

		for(size_t i = 0; i < data.size(); ++i) {
			if(linectr == line) {
				found = true;
				idx   = i;
				break;
			}
			if(data[i] == '\n') {
				++linectr;
				continue;
			}
		}
		std::string err_line = "<not found>";
		if(found) {
			size_t count = data.find('\n', idx);
			if(count != std::string::npos) count -= idx;
			err_line = data.substr(idx, count);
		}

		size_t tab_count = 0;
		for(auto &c : err_line) {
			if(c == '\t') ++tab_count;
		}
		std::string spacing_caret(col /* + 1 for single character '^' */, ' ');
		while(tab_count--) {
			spacing_caret.pop_back();
			spacing_caret.insert(spacing_caret.begin(), '\t');
		}

		fprintf(out, "%s (%zu:%zu): %s: %s\n%s\n%s%c\n", filename.c_str(), line + 1,
			col + 1, warn ? "Warning" : "Failure", e.c_str(), err_line.c_str(),
			spacing_caret.c_str(), '^');

		locs.pop_back();
		errs.pop_back();
		warns.pop_back();
	}
}
void ErrMgr::reset()
{
	locs.clear();
	errs.clear();
	warns.clear();
}
} // namespace sc