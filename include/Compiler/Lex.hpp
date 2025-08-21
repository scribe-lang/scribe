#pragma once

#include "Core.hpp"

namespace sc::lex
{

// true => @as(i1, 1)
// false => @as(i1, 0)
// nil => @as(usize, 0)
enum TokType
{
	// Special data/types (nil, true, false, void, any, type, etc.) fall under iden
	INT,
	FLT,
	STR,
	CHAR,
	IDEN,

	// Special
	ATTRS,

	// Keywords
	LET,
	FN,
	IF,
	ELIF,
	ELSE,
	GOTO,
	FOR,
	IN,
	WHILE,
	RETURN,
	CONTINUE,
	BREAK,
	OR,
	STATIC,
	CONST,
	VOLATILE,
	DEFER,
	EXTERN,
	COMPTIME,
	GLOBAL,
	INLINE,

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

inline bool isTokInt(TokType token) { return token == INT; }
inline bool isTokFlt(TokType token) { return token == FLT; }
inline bool isTokLiteral(TokType token)
{
	return token == INT || token == FLT || token == CHAR || token == STR;
}
inline bool isTokIdentifier(TokType token) { return token == IDEN; }
inline bool isTokNumeric(TokType token) { return isTokInt(token) || isTokFlt(token); }
inline bool isTokOper(TokType token) { return token >= ASSN && token <= RBRACK; }

inline bool isTokUnaryPre(TokType token)
{
	return token == UADD || token == USUB || token == UAND || token == UMUL || token == INCX ||
	       token == DECX || token == LNOT || token == BNOT;
}

inline bool isTokUnaryPost(TokType token) { return token == XINC || token == XDEC; }

inline bool isTokComparison(TokType token)
{
	return token == LT || token == GT || token == LE || token == GE || token == EQ ||
	       token == NE;
}

inline bool isTokAssign(TokType token)
{
	return (token == ASSN || token == ADD_ASSN || token == SUB_ASSN || token == MUL_ASSN ||
		token == DIV_ASSN || token == MOD_ASSN || token == BAND_ASSN || token == BOR_ASSN ||
		token == BNOT_ASSN || token == BXOR_ASSN || token == LSHIFT_ASSN ||
		token == RSHIFT_ASSN);
}

inline bool isTokValid(TokType token) { return token != INVALID && token != FEOF; }

inline const char *tokToCStr(TokType token) { return TokStrs[token]; }

const char *getTokUnaryNoCharCStr(TokType token);
const char *getTokOperCStr(TokType token);

class Lexeme
{
	fs::FileLoc loc;
	TokType tokty;
	Variant<String, int64_t, long double> data;
	bool containsdata;

public:
	Lexeme(fs::FileLoc loc = fs::FileLoc());
	explicit Lexeme(fs::FileLoc loc, TokType type);
	explicit Lexeme(fs::FileLoc loc, TokType type, StringRef _data);
	explicit Lexeme(fs::FileLoc loc, TokType type, int64_t _data);
	explicit Lexeme(fs::FileLoc loc, TokType type, long double _data);

	String str(int64_t pad = 10) const;

	bool operator==(const Lexeme &other) const;
	inline bool operator!=(const Lexeme &other) const { return *this == other ? false : true; }

	inline void setDataStr(StringRef str) { std::get<String>(data) = str; }
	inline void setDataStr(String &&str)
	{
		using namespace std;
		swap(std::get<String>(data), str);
	}
	inline void setDataInt(int64_t i) { data = i; }
	inline void setDataFlt(long double f) { data = f; }

	inline StringRef getDataStr() const { return std::get<String>(data); }
	inline int64_t getDataInt() const { return std::get<int64_t>(data); }
	inline long double getDataFlt() const { return std::get<long double>(data); }

	inline TokType getType() const { return tokty; }
	inline bool isType(TokType ty) const { return tokty == ty; }
	inline void setType(TokType ty) { tokty = ty; }
	inline const fs::FileLoc &getLoc() const { return loc; }
	inline bool containsData() const { return containsdata; }

	inline bool isInt() const { return isTokInt(tokty) && containsdata; }
	inline bool isFlt() const { return isTokFlt(tokty) && containsdata; }
	inline bool isLiteral() const { return isTokLiteral(tokty) && containsdata; }
	inline bool isIdentifier() const { return isTokIdentifier(tokty) && containsdata; }
	inline bool isNumeric() const { return isInt() || isFlt(); }
	inline bool isOper() const { return isTokOper(tokty); }
	inline bool isUnaryPre() const { return isTokUnaryPre(tokty); }
	inline bool isUnaryPost() const { return isTokUnaryPost(tokty); }
	inline bool isComparison() const { return isTokComparison(tokty); }
	inline bool isAssign() const { return isTokAssign(tokty); }
	inline bool isValid() const { return isTokValid(tokty); }
	inline const char *getUnaryNoCharCStr() const { return getTokUnaryNoCharCStr(tokty); }
	inline const char *getOperCStr() const { return getTokOperCStr(tokty); }
	inline const char *tokCStr() const { return tokToCStr(tokty); }
	inline StringRef tokStr() const { return tokToCStr(tokty); }
};

bool tokenize(size_t moduleId, StringRef data, Vector<Lexeme> &toks);
void dumpTokens(OStream &os, Span<Lexeme> toks);

} // namespace sc::lex
