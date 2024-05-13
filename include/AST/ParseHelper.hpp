#pragma once

#include "Lex.hpp"
#include "Parser.hpp"

namespace sc::AST
{
class ParseHelper
{
	Context &ctx;
	Module *mod;
	// requires modification at parsing stage, hence not set by module pointer
	Vector<lex::Lexeme> &toks;
	ModuleLoc *emptyloc; // used by invalid and eof
	lex::Lexeme invalid, eof;
	size_t idx;

public:
	ParseHelper(Context &ctx, Module *mod, Vector<lex::Lexeme> &toks, size_t begin = 0);

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

	inline bool accept(lex::TokType type) { return peekt() == type; }
	inline bool accept(lex::TokType t1, lex::TokType t2)
	{
		lex::TokType t = peekt();
		return t == t1 || t == t2;
	}
	inline bool accept(lex::TokType t1, lex::TokType t2, lex::TokType t3)
	{
		lex::TokType t = peekt();
		return t == t1 || t == t2 || t == t3;
	}

	inline bool acceptn(lex::TokType type)
	{
		if(accept(type)) {
			next();
			return true;
		}
		return false;
	}
	inline bool acceptn(lex::TokType t1, lex::TokType t2)
	{
		if(accept(t1, t2)) {
			next();
			return true;
		}
		return false;
	}
	inline bool acceptn(lex::TokType t1, lex::TokType t2, lex::TokType t3)
	{
		if(accept(t1, t2, t3)) {
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

	inline Module *getModule() { return mod; }

	inline bool hasNext() const { return idx + 1 < toks.size(); }

	inline void setPos(size_t idx) { this->idx = idx; }
	inline size_t getPos() const { return idx; }
};
} // namespace sc::AST
