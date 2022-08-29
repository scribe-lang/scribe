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

#ifndef ERROR_HPP
#define ERROR_HPP

#include "Core.hpp"

namespace sc
{
namespace lex
{
class Lexeme;
} // namespace lex
class Module;
class Stmt;

class ModuleLoc
{
	Module *mod;
	size_t line;
	size_t col;

public:
	ModuleLoc(Module *mod, const size_t &line, const size_t &col);

	String getLocStr() const;
	inline Module *getMod() const { return mod; }
	inline size_t getLine() const { return line; }
	inline size_t getCol() const { return col; }
};

namespace err
{

void setMaxErrs(size_t max_err);

void out(Stmt *stmt, InitList<StringRef> err);
void out(const lex::Lexeme &tok, InitList<StringRef> err);
void out(const ModuleLoc &loc, InitList<StringRef> err);

// equivalent to out(), but for warnings
void outw(Stmt *stmt, InitList<StringRef> err);
void outw(const lex::Lexeme &tok, InitList<StringRef> err);
void outw(const ModuleLoc &loc, InitList<StringRef> err);

} // namespace err
} // namespace sc

#endif // ERROR_HPP