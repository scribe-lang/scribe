/*
	MIT License

	Copyright (c) 2021 Scribe Language Repositories

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so.
*/

#ifndef ERROR_HPP
#define ERROR_HPP

#include <cassert>
#include <cstdarg>
#include <string>
#include <vector>

namespace sc
{
namespace lex
{
class Lexeme;
}
class Module;
class Stmt;

class ModuleLoc
{
	Module *mod;
	size_t line;
	size_t col;

public:
	ModuleLoc(Module *mod, const size_t &line, const size_t &col);

	std::string getLocStr() const;
	inline Module *getMod() const
	{
		return mod;
	}

	friend class ErrMgr;
};

class ErrMgr
{
	std::vector<ModuleLoc> locs;
	std::vector<std::string> errs;
	std::vector<bool> warns;

public:
	void set(Stmt *stmt, const char *e, ...);
	void set(const lex::Lexeme &tok, const char *e, ...);
	void set(const ModuleLoc &loc, const char *e, va_list args);

	// equivalent to set(), but for warnings
	void setw(Stmt *stmt, const char *e, ...);
	void setw(const lex::Lexeme &tok, const char *e, ...);
	void setw(const ModuleLoc &loc, const char *e, va_list args);
	bool present();
	void show(FILE *f);
	void reset();
};
} // namespace sc

#endif // ERROR_HPP