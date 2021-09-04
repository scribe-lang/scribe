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

#ifndef PARSER_PARSE_HELPER_HPP
#define PARSER_PARSE_HELPER_HPP

#include "Lex.hpp"
#include "Parser.hpp"

namespace sc
{
class ParseHelper
{
	Module *mod;
	// requires modification at parsing stage, hence not set by module pointer
	std::vector<lex::Lexeme> &toks;
	lex::Lexeme invalid, eof;
	size_t idx;

public:
	ParseHelper(Module *mod, std::vector<lex::Lexeme> &toks, const size_t &begin = 0);

	lex::Lexeme &peak(const int offset = 0);
	lex::TokType peakt(const int offset = 0) const;

	lex::Lexeme &next();
	lex::TokType nextt();

	lex::Lexeme &prev();
	lex::TokType prevt();

	inline void sett(const lex::TokType type)
	{
		if(idx < toks.size()) toks[idx].getTok().setVal(type);
	}

	inline bool accept(const lex::TokType type)
	{
		return peakt() == type;
	}
	inline bool accept(const lex::TokType t1, const lex::TokType t2)
	{
		const lex::TokType t = peakt();
		return t == t1 || t == t2;
	}
	inline bool accept(const lex::TokType t1, const lex::TokType t2, const lex::TokType t3)
	{
		const lex::TokType t = peakt();
		return t == t1 || t == t2 || t == t3;
	}

	inline bool acceptn(const lex::TokType type)
	{
		if(accept(type)) {
			next();
			return true;
		}
		return false;
	}
	inline bool acceptn(const lex::TokType t1, const lex::TokType t2)
	{
		if(accept(t1, t2)) {
			next();
			return true;
		}
		return false;
	}
	inline bool acceptn(const lex::TokType t1, const lex::TokType t2, const lex::TokType t3)
	{
		if(accept(t1, t2, t3)) {
			next();
			return true;
		}
		return false;
	}

	inline bool acceptd()
	{
		return peak().getTok().isData();
	}

	inline bool isValid()
	{
		return !accept(lex::INVALID, lex::FEOF);
	}

	const lex::Lexeme *at(const size_t &idx) const;

	inline Module *getModule()
	{
		return mod;
	}

	inline bool hasNext() const
	{
		return idx + 1 < toks.size();
	}

	inline void setPos(const size_t &idx)
	{
		this->idx = idx;
	}
	inline size_t getPos() const
	{
		return idx;
	}
};
} // namespace sc

#endif // PARSER_PARSE_HELPER_HPP