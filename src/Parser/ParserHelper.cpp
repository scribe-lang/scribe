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

#include "Parser/ParseHelper.hpp"

namespace sc
{
ParseHelper::ParseHelper(Module *mod, std::vector<lex::Lexeme> &toks, const size_t &begin)
	: mod(mod), toks(toks), invalid(ModuleLoc(mod, 0, 0), lex::INVALID),
	  eof(ModuleLoc(mod, 0, 0), lex::FEOF), idx(begin)
{}

lex::Lexeme &ParseHelper::peak(const int offset)
{
	if(offset < 0 && idx < (-offset)) return eof;
	if(idx + offset >= toks.size()) return eof;
	return toks[idx + offset];
}

lex::TokType ParseHelper::peakt(const int offset) const
{
	if(offset < 0 && idx < (-offset)) return eof.getTokVal();
	if(idx + offset >= toks.size()) return eof.getTokVal();
	return toks[idx + offset].getTokVal();
}

lex::Lexeme &ParseHelper::next()
{
	++idx;
	if(idx >= toks.size()) return eof;
	return toks[idx];
}

lex::TokType ParseHelper::nextt()
{
	++idx;
	if(idx >= toks.size()) return eof.getTokVal();
	return toks[idx].getTokVal();
}

lex::Lexeme &ParseHelper::prev()
{
	if(idx == 0) return invalid;
	--idx;
	return toks[idx];
}

lex::TokType ParseHelper::prevt()
{
	if(idx == 0) return invalid.getTokVal();
	--idx;
	return toks[idx].getTokVal();
}

const lex::Lexeme *ParseHelper::at(const size_t &idx) const
{
	if(idx >= toks.size()) return &invalid;
	return &toks[idx];
}
} // namespace sc