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

#ifndef LEX_HPP
#define LEX_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "Error.hpp"

namespace sc
{
namespace lex
{
enum TokType
{
	INT,
	FLT,

	CHAR,
	STR,
	IDEN,

	// Keywords
	LET,
	FN,
	IF,
	ELIF,
	ELSE,
	FOR,
	IN,
	WHILE,
	RETURN,
	CONTINUE,
	BREAK,
	VOID,
	TRUE,
	FALSE,
	NIL,
	ANY,  // type: any
	TYPE, // type: type
	I1,
	I8,
	I16,
	I32,
	I64,
	U8,
	U16,
	U32,
	U64,
	F32,
	F64,
	OR,
	STATIC,
	CONST,
	VOLATILE,
	DEFER,
	EXTERN,
	COMPTIME,
	GLOBAL,
	INLINE,
	STRUCT,
	ENUM,

	// Operators
	ASSN,
	// Arithmetic
	ADD,
	SUB,
	MUL,
	DIV,
	MOD,
	ADD_ASSN,
	SUB_ASSN,
	MUL_ASSN,
	DIV_ASSN,
	MOD_ASSN,
	// Post/Pre Inc/Dec
	XINC,
	INCX,
	XDEC,
	DECX,
	// Unary
	UADD,
	USUB,
	UAND, // address of
	UMUL, // dereference
	// Logic
	LAND,
	LOR,
	LNOT,
	// Comparison
	EQ,
	LT,
	GT,
	LE,
	GE,
	NE,
	// Bitwise
	BAND,
	BOR,
	BNOT,
	BXOR,
	BAND_ASSN,
	BOR_ASSN,
	BNOT_ASSN,
	BXOR_ASSN,
	// Others
	LSHIFT,
	RSHIFT,
	LSHIFT_ASSN,
	RSHIFT_ASSN,

	SUBS,

	FNCALL, // function call and struct template specialization
	STCALL, // instantiate structs

	// Varargs
	PreVA,
	PostVA,

	// Separators
	DOT,
	QUEST,
	COL,
	COMMA,
	AT,
	SPC,
	TAB,
	NEWL,
	COLS, // Semi colon
	ARROW,
	// Parenthesis, Braces, Brackets
	LPAREN,
	RPAREN,
	LBRACE,
	RBRACE,
	LBRACK,
	RBRACK,

	FEOF,
	INVALID,

	_LAST,
};

/**
 * \brief String value of each of the lexical tokens
 */
extern const char *TokStrs[_LAST];

class Tok
{
	TokType val;

public:
	Tok(const int &tok);

	inline bool isData() const
	{
		return val == INT || val == FLT || val == CHAR || val == STR || val == IDEN ||
		       val == VOID || val == TRUE || val == FALSE || val == NIL || val == ANY ||
		       val == TYPE || val == I1 || val == I8 || val == I16 || val == I32 ||
		       val == I64 || val == U8 || val == U16 || val == U32 || val == U64 ||
		       val == F32 || val == F64;
	}

	inline bool isOper() const
	{
		return val >= ASSN && val <= RBRACK;
	}

	inline bool isUnaryPre() const
	{
		return val == UADD || val == USUB || val == UAND || val == UMUL || val == INCX ||
		       val == DECX || val == LNOT || val == BNOT;
	}

	inline bool isUnaryPost() const
	{
		return val == XINC || val == XDEC;
	}

	inline bool isAssign() const
	{
		return (val == ASSN || val == ADD_ASSN || val == SUB_ASSN || val == MUL_ASSN ||
			val == DIV_ASSN || val == MOD_ASSN || val == BAND_ASSN || val == BOR_ASSN ||
			val == BNOT_ASSN || val == BXOR_ASSN || val == LSHIFT_ASSN ||
			val == RSHIFT_ASSN);
	}

	inline bool isValid() const
	{
		return val != INVALID && val != FEOF;
	}

	inline const char *cStr() const
	{
		return TokStrs[val];
	}
	inline std::string str() const
	{
		return TokStrs[val];
	}

	const char *getOperCStr() const;
	const char *getUnaryNoCharCStr() const;

	inline bool operator==(const Tok &other) const
	{
		return val == other.val;
	}

	inline TokType getVal() const
	{
		return val;
	}

	inline void setVal(const TokType &v)
	{
		val = v;
	}

	inline bool isType(const TokType &other) const
	{
		return val == other;
	}
};

struct Data
{
	std::string s;
	int64_t i;
	long double f;

	bool cmp(const Data &other, const TokType type) const;
};

class Lexeme
{
	ModuleLoc loc;
	Tok tok;
	Data data;

public:
	Lexeme(const ModuleLoc &loc = ModuleLoc(nullptr, 0, 0));
	explicit Lexeme(const ModuleLoc &loc, const TokType &type);
	explicit Lexeme(const ModuleLoc &loc, const TokType &type, const std::string &_data);
	explicit Lexeme(const ModuleLoc &loc, const int64_t &_data);
	explicit Lexeme(const ModuleLoc &loc, const long double &_data);

	std::string str(const int64_t &pad = 10) const;

	inline bool operator==(const Lexeme &other) const
	{
		return tok == other.tok && data.cmp(other.data, tok.getVal());
	}
	inline bool operator!=(const Lexeme &other) const
	{
		return *this == other ? false : true;
	}

	inline void setDataStr(const std::string &str)
	{
		data.s = str;
	}
	inline void setDataInt(const int64_t &i)
	{
		data.i = i;
	}
	inline void setDataFlt(const long double &f)
	{
		data.f = f;
	}

	inline const std::string &getDataStr() const
	{
		return data.s;
	}
	inline const int64_t &getDataInt() const
	{
		return data.i;
	}
	inline const long double &getDataFlt() const
	{
		return data.f;
	}

	inline Tok &getTok()
	{
		return tok;
	}
	inline const Tok &getTok() const
	{
		return tok;
	}
	inline TokType getTokVal() const
	{
		return tok.getVal();
	}
	inline const ModuleLoc &getLoc() const
	{
		return loc;
	}
};

class Tokenizer
{
	Module *mod;
	ErrMgr &err;

	ModuleLoc loc(const size_t &line, const size_t &col);

	std::string get_name(const std::string &data, size_t &i);
	TokType classify_str(const std::string &str);
	std::string get_num(const std::string &data, size_t &i, size_t &line, size_t &line_start,
			    TokType &num_type, int &base);
	bool get_const_str(const std::string &data, char &quote_type, size_t &i, size_t &line,
			   size_t &line_start, std::string &buf);
	TokType get_operator(const std::string &data, size_t &i, const size_t &line,
			     const size_t &line_start);
	void remove_back_slash(std::string &s);

public:
	Tokenizer(Module *m, ErrMgr &e);
	bool tokenize(const std::string &data, std::vector<Lexeme> &toks);
};
} // namespace lex
} // namespace sc

#endif // LEX_HPP