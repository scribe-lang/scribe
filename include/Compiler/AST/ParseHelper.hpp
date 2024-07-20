#pragma once

#include "Lex.hpp"

namespace sc::ast
{
class ParseHelper
{
	// requires modification at parsing stage, hence not set by module pointer
	Vector<lex::Lexeme> &toks;
	lex::Lexeme invalid, eof;
	size_t idx;

public:
	ParseHelper(Vector<lex::Lexeme> &toks, size_t begin = 0);

	lex::Lexeme &peek(int offset = 0);
	lex::TokType peekt(int offset = 0) const;

	lex::Lexeme &next();
	lex::TokType nextt();

	lex::Lexeme &prev();
	lex::TokType prevt();

	inline void sett(lex::TokType type)
	{
		if(idx < toks.size()) toks[idx].setType(type);
	}

	// Takes variadic of lex::TokType
	template<typename... Args> bool accept(Args... types)
	{
		// Fold expression magic :D
		lex::TokType t = peekt();
		bool res       = false;
		([&] { res |= t == types; }(), ...);
		return res;
	}

	// Takes variadic of lex::TokType
	template<typename... Args> bool acceptn(Args... types)
	{
		if(accept(types...)) {
			next();
			return true;
		}
		return false;
	}

	inline bool acceptd()
	{
		return peek().isLiteral() || peek().isIdentifier() || peek().isType();
	}

	inline bool isValid() { return !accept(lex::INVALID, lex::FEOF); }

	const lex::Lexeme *at(size_t idx) const;

	inline bool hasNext() const { return idx + 1 < toks.size(); }

	inline void setPos(size_t idx) { this->idx = idx; }
	inline size_t getPos() const { return idx; }
};
} // namespace sc::ast
