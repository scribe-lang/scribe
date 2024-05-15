#include "Lex.hpp"

#include <charconv>
#include <limits>

namespace sc
{
namespace lex
{
const char *TokStrs[_LAST] = {
"i1",
"i8",
"i16",
"i32",
"i64",
"u8",
"u16",
"u32",
"u64",
"f32",
"f64",
"void",
"STR",
"IDEN",
"ATTRS",

// Keywords
"let",
"fn",
"if",
"elif",
"else",
"for",
"in",
"while",
"return",
"continue",
"break",
"any",
"type",
"or",
"static",
"const",
"volatile",
"defer",
"extern",
"comptime",
"global",
"inline",
"struct",
"union",
"enum",

// Operators
"=",
// Arithmetic
"+",
"-",
"*",
"/",
"%",
"+=",
"-=",
"*=",
"/=",
"%=",
// Post/Pre Inc/Dec
"x++",
"++x",
"x--",
"--x",
// Unary (used by parser (in Expression.cpp))
"u+",
"u-",
"u&",
"u*",
// Logic
"&&",
"||",
"!",
// Comparison
"==",
"<",
">",
"<=",
">=",
"!=",
// Bitwise
"&",
"|",
"~",
"^",
"&=",
"|=",
"~=",
"^=",
// Others
"<<",
">>",
"<<=",
">>=",

"[]",
"()",
"{}",

// Varargs
"...x",
"x...",

// Separators
".",
"?",
":",
",",
"@",
"SPC",
"TAB",
"NEWL",
";",
"->",
// Parenthesis, Braces, Brackets
"(",
")",
"{",
"}",
"[",
"]",

"<FEOF>",
"<INVALID>",
};

String viewBackSlash(StringRef data);

const char *getTokUnaryNoCharCStr(TokType token)
{
	switch(token) {
	case XINC:
	case INCX: return "++";
	case XDEC:
	case DECX: return "--";
	case UADD: return "+";
	case USUB: return "-";
	case UAND: return "&";
	case UMUL: return "*";
	case LNOT: return "!";
	case BNOT: return "~";
	default: break;
	}
	return "";
}

const char *getTokOperCStr(TokType token)
{
	switch(token) {
	case ASSN: return "__assn__";
	// Arithmetic
	case ADD: return "__add__";
	case SUB: return "__sub__";
	case MUL: return "__mul__";
	case DIV: return "__div__";
	case MOD: return "__mod__";
	case ADD_ASSN: return "__add_assn__";
	case SUB_ASSN: return "__sub_assn__";
	case MUL_ASSN: return "__mul_assn__";
	case DIV_ASSN: return "__div_assn__";
	case MOD_ASSN: return "__mod_assn__";
	// Post/Pre Inc/Dec
	case XINC: return "__xinc__";
	case INCX: return "__incx__";
	case XDEC: return "__xdec__";
	case DECX: return "__decx__";
	// Unary
	case UADD: return "__uadd__";
	case USUB: return "__usub__";
	// Logic
	case LAND: return "__logand__";
	case LOR: return "__logor__";
	case LNOT: return "__lognot__";
	// Comparison
	case EQ: return "__eq__";
	case LT: return "__lt__";
	case GT: return "__gt__";
	case LE: return "__le__";
	case GE: return "__ge__";
	case NE: return "__ne__";
	// Bitwise
	case BAND: return "__band__";
	case BOR: return "__bor__";
	case BNOT: return "__bnot__";
	case BXOR: return "__bxor__";
	case BAND_ASSN: return "__band_assn__";
	case BOR_ASSN: return "__bor_assn__";
	case BNOT_ASSN: return "__bnot_assn__";
	case BXOR_ASSN: return "__bxor_assn__";
	// Others
	case LSHIFT: return "__lshift__";
	case RSHIFT: return "__rshift__";
	case LSHIFT_ASSN: return "__lshift_assn__";
	case RSHIFT_ASSN: return "__rshift_assn__";
	case SUBS: return "__subscr__";
	default: break;
	}
	return "";
}

uint64_t getIntMaxForType(TokType token)
{
	if(token == I1) return std::numeric_limits<bool>().max();
	if(token == I8) return std::numeric_limits<int8_t>().max();
	if(token == I16) return std::numeric_limits<int16_t>().max();
	if(token == I32) return std::numeric_limits<int32_t>().max();
	if(token == I64) return std::numeric_limits<int64_t>().max();
	if(token == I8) return std::numeric_limits<uint8_t>().max();
	if(token == I16) return std::numeric_limits<uint16_t>().max();
	if(token == I32) return std::numeric_limits<uint32_t>().max();
	if(token == I64) return std::numeric_limits<uint64_t>().max();
	return 0;
}
int64_t getIntMinForType(TokType token)
{
	if(token == I1) return std::numeric_limits<bool>().min();
	if(token == I8) return std::numeric_limits<int8_t>().min();
	if(token == I16) return std::numeric_limits<int16_t>().min();
	if(token == I32) return std::numeric_limits<int32_t>().min();
	if(token == I64) return std::numeric_limits<int64_t>().min();
	if(token == I8) return std::numeric_limits<uint8_t>().min();
	if(token == I16) return std::numeric_limits<uint16_t>().min();
	if(token == I32) return std::numeric_limits<uint32_t>().min();
	if(token == I64) return std::numeric_limits<uint64_t>().min();
	return 0;
}
long double getFltMaxForType(TokType token)
{
	if(token == F32) return std::numeric_limits<float>().max();
	if(token == F64) return std::numeric_limits<long double>().max();
	return 0.0;
}
long double getFltMinForType(TokType token)
{
	if(token == F32) return std::numeric_limits<float>().min();
	if(token == F64) return std::numeric_limits<long double>().min();
	return 0.0;
}

Lexeme::Lexeme(const ModuleLoc *loc) : loc(loc), tokty(INVALID), data(0), containsdata(false) {}
Lexeme::Lexeme(const ModuleLoc *loc, TokType type)
	: loc(loc), tokty(type), data(0), containsdata(false)
{}
Lexeme::Lexeme(const ModuleLoc *loc, TokType type, StringRef _data)
	: loc(loc), tokty(type), data(String(_data)), containsdata(true)
{}
Lexeme::Lexeme(const ModuleLoc *loc, TokType type, int64_t _data)
	: loc(loc), tokty(type), data(_data), containsdata(true)
{}
Lexeme::Lexeme(const ModuleLoc *loc, TokType type, long double _data)
	: loc(loc), tokty(type), data(_data), containsdata(true)
{}
String Lexeme::str(int64_t pad) const
{
	String res;
	int64_t len;
	res += tokCStr();
	len = res.size();
	for(int64_t i = 0; i < pad - len; ++i) res += " ";
	if(pad == 0) res += " ";
	len = res.size();
	res += "[" + loc->getLocStr() + "]";
	if(!containsData()) return res;
	len = res.size() - len;
	for(int64_t i = 0; i < pad - len; ++i) res += " ";
	if(pad == 0) res += " ";
	if(tokty == STR || tokty == IDEN) {
		res += viewBackSlash(std::get<String>(data));
	} else if(tokty >= I1 && tokty <= U64) {
		res += std::to_string(std::get<int64_t>(data));
	} else if(tokty >= F32 && tokty <= F64) {
		res += std::to_string(std::get<long double>(data));
	}
	return res;
}
bool Lexeme::operator==(const Lexeme &other) const
{
	if(tokty != other.tokty) return false;
	if(tokty == STR || tokty == IDEN) {
		return std::get<String>(data) == std::get<String>(other.data);
	} else if(tokty >= I1 && tokty <= U64) {
		return std::get<int64_t>(data) == std::get<int64_t>(other.data);
	} else if(tokty >= F32 && tokty <= F64) {
		return std::get<long double>(data) == std::get<long double>(other.data);
	}
	return false;
}

#define CURR (data[i])
#define NEXT (i + 1 < len ? data[i + 1] : 0)
#define PREV (len > 0 && i > 0 ? data[i - 1] : 0)
#define SET_OP_TYPE_BRK(type) \
	op_type = type;       \
	break

Tokenizer::Tokenizer(Context &ctx, Module *m) : ctx(ctx), mod(m) {}

ModuleLoc *Tokenizer::locAlloc(size_t line, size_t col)
{
	return ctx.allocModuleLoc(mod, line, col);
}
ModuleLoc Tokenizer::loc(size_t line, size_t col) { return ModuleLoc(mod, line, col); }

bool Tokenizer::tokenize(StringRef data, Vector<Lexeme> &toks)
{
	int comment_block = 0; // int to handle nested comment blocks
	bool comment_line = false;

	size_t len	  = data.size();
	size_t i	  = 0;
	size_t line	  = 0;
	size_t line_start = 0;
	while(i < len) {
		if(CURR == '\n') {
			++line;
			line_start = i + 1;
		}
		if(comment_line) {
			if(CURR == '\n') comment_line = false;
			++i;
			continue;
		}
		if(isspace(CURR)) {
			++i;
			continue;
		}
		if(CURR == '*' && NEXT == '/') {
			if(!comment_block) {
				err::out(loc(line, i - line_start),
					 "encountered multi line comment "
					 "terminator '*/' in non comment block");
				return false;
			}
			i += 2;
			--comment_block;
			continue;
		}
		if(CURR == '/' && NEXT == '*') {
			i += 2;
			++comment_block;
			continue;
		}
		if(comment_block) {
			++i;
			continue;
		}
		if(CURR == '/' && NEXT == '/') {
			comment_line = true;
			++i;
			continue;
		}

		// attributes
		if(CURR == '#') {
			StringRef attrs = getAttributes(data, ++i);
			toks.emplace_back(locAlloc(line, i - line_start - attrs.size()), ATTRS,
					  attrs);
			continue;
		}

		// identifiers
		if((CURR == '.' && (isalpha(NEXT) || NEXT == '_') && !isalnum(PREV) &&
		    PREV != '_' && PREV != ')' && PREV != ']' && PREV != '\'' && PREV != '"') ||
		   isalpha(CURR) || CURR == '_')
		{
			StringRef str = getName(data, i);
			// true and false are handled separately - they are technically keywords,
			// but they can also be represented as i1(1) and i1(0) respectively
			if(str == "true" || str == "false") {
				toks.emplace_back(locAlloc(line, i - line_start - str.size()), I1,
						  (int64_t)(str == "true"));
				continue;
			}
			// check if string is a keyword
			TokType str_class = classifyStr(str);
			if(str[0] == '.') str = str.substr(0, 1);
			if(str_class == STR || str_class == IDEN) {
				// place either the data itself (type = STR, IDEN)
				toks.emplace_back(locAlloc(line, i - line_start - str.size()),
						  str_class, str);
			} else { // or the type
				toks.emplace_back(locAlloc(line, i - line_start - str.size()),
						  str_class);
			}
			continue;
		}

		// numbers
		if(isdigit(CURR)) {
			TokType num_type = I32;
			int base	 = 10;
			String num	 = getNum(data, i, line, line_start, num_type, base);
			if(num.empty()) return false;
			if(num_type == F32 || num_type == F64) {
				long double fltval;
				std::from_chars(num.data(), num.data() + num.size(), fltval);
				toks.emplace_back(locAlloc(line, i - line_start - num.size()),
						  num_type, fltval);
				continue;
			}
			int64_t intval;
			if(num.size() > 2 && base != 10) {
				// base of 8: starts with 0 => 0755
				// everything else: starts with 0 and letter
				num = num.substr(base == 8 ? 1 : 2);
			}
			std::from_chars(num.data(), num.data() + num.size(), intval, base);
			toks.emplace_back(locAlloc(line, i - line_start - num.size()), num_type,
					  intval);
			continue;
		}

		// const strings
		if(CURR == '\"' || CURR == '\'' || CURR == '`') {
			String str;
			char quote_type = 0;
			if(!getConstStr(data, quote_type, i, line, line_start, str)) return false;
			if(quote_type == '\'') {
				toks.emplace_back(locAlloc(line, i - line_start - str.size()), I8,
						  (int64_t)str[0]);
			} else {
				toks.emplace_back(locAlloc(line, i - line_start - str.size()), STR,
						  str);
			}
			continue;
		}

		// operators
		size_t begin	= i;
		TokType op_type = getOperator(data, i, line, line_start);
		if(op_type == INVALID) return false;
		toks.emplace_back(locAlloc(line, begin - line_start), op_type);
	}
	return true;
}

StringRef Tokenizer::getAttributes(StringRef data, size_t &i)
{
	size_t len   = data.size();
	size_t start = i;
	while(i < len) {
		if(!isalnum(CURR) && CURR != ',' && CURR != '=' && CURR != '_') break;
		++i;
	}
	return data.substr(start, i - start);
}

StringRef Tokenizer::getName(StringRef data, size_t &i)
{
	size_t len   = data.size();
	size_t start = i;
	while(i < len) {
		if(!isalnum(CURR) && CURR != '_') break;
		++i;
	}
	if(i < len && CURR == '?') ++i;
	return data.substr(start, i - start);
}

TokType Tokenizer::classifyStr(StringRef str)
{
	if(str == TokStrs[GLOBAL]) return GLOBAL;
	if(str == TokStrs[LET]) return LET;
	if(str == TokStrs[FN]) return FN;
	if(str == TokStrs[IF]) return IF;
	if(str == TokStrs[ELIF]) return ELIF;
	if(str == TokStrs[ELSE]) return ELSE;
	if(str == TokStrs[FOR]) return FOR;
	if(str == TokStrs[IN]) return IN;
	if(str == TokStrs[WHILE]) return WHILE;
	if(str == TokStrs[RETURN]) return RETURN;
	if(str == TokStrs[CONTINUE]) return CONTINUE;
	if(str == TokStrs[BREAK]) return BREAK;
	if(str == TokStrs[VOID]) return VOID;
	if(str == TokStrs[ANY]) return ANY;
	if(str == TokStrs[TYPE]) return TYPE;
	if(str == TokStrs[I1]) return I1;
	if(str == TokStrs[I8]) return I8;
	if(str == TokStrs[I16]) return I16;
	if(str == TokStrs[I32]) return I32;
	if(str == TokStrs[I64]) return I64;
	if(str == TokStrs[U8]) return U8;
	if(str == TokStrs[U16]) return U16;
	if(str == TokStrs[U32]) return U32;
	if(str == TokStrs[U64]) return U64;
	if(str == TokStrs[F32]) return F32;
	if(str == TokStrs[F64]) return F64;
	if(str == TokStrs[OR]) return OR;
	if(str == TokStrs[STATIC]) return STATIC;
	if(str == TokStrs[CONST]) return CONST;
	if(str == TokStrs[VOLATILE]) return VOLATILE;
	if(str == TokStrs[DEFER]) return DEFER;
	if(str == TokStrs[EXTERN]) return EXTERN;
	if(str == TokStrs[COMPTIME]) return COMPTIME;
	if(str == TokStrs[INLINE]) return INLINE;
	if(str == TokStrs[STRUCT]) return STRUCT;
	if(str == TokStrs[UNION]) return UNION;
	if(str == TokStrs[ENUM]) return ENUM;

	// if string begins with dot, it's an atom (str), otherwise an identifier
	return str[0] == '.' ? STR : IDEN;
}

String Tokenizer::getNum(StringRef data, size_t &i, size_t &line, size_t &line_start,
			 TokType &num_type, int &base)
{
	size_t len = data.size();
	String buf;
	size_t first_digit_at = i;

	int dot_loc = -1;
	base	    = 10;

	bool read_base = false;

	while(i < len) {
		const char c	= CURR;
		const char next = NEXT;
		switch(c) {
		case 'x':
		case 'X': {
			if(read_base) {
				base	  = 16;
				read_base = false;
				break;
			}
			goto fail;
		}
		case 'f':
		case 'F':
		case 'e':
		case 'E':
		case 'd':
		case 'D':
		case 'c':
		case 'C':
		case 'b':
		case 'B':
		case 'a':
		case 'A': {
			if(base >= 16) break;
			goto fail;
		}
		case '9':
		case '8':
		case '7':
			if(base >= 8) break;
			goto fail;
		case '6':
		case '5':
		case '4':
		case '3':
		case '2':
			if(base > 2) break;
			goto fail;
		case '1': break;
		case '0': {
			if(i == first_digit_at) {
				read_base = true;
				base	  = 8;
			}
			break;
		}
		case '.':
			if(!read_base && base != 10) {
				err::out(loc(line, first_digit_at - line_start),
					 "encountered dot (.) character when base is not 10 (",
					 base, ") ");
				return "";
			} else if(dot_loc == -1) {
				if(next >= '0' && next <= '9') {
					dot_loc	 = i;
					num_type = F32;
				} else {
					goto end;
				}
			} else {
				err::out(loc(line, first_digit_at - line_start),
					 "encountered dot (.) character when the number being "
					 "retrieved (from column ",
					 first_digit_at + 1, ") already had one");
				return "";
			}
			read_base = false;
			base	  = 10;
			break;
		default:
		fail:
			if(isalnum(c)) {
				err::out(loc(line, first_digit_at - line_start),
					 "encountered invalid character '", c,
					 "' while retrieving a number of base ", base);
				return "";
			} else {
				goto end;
			}
		}
		if(!buf.empty() || c != '0') read_base = false;
		buf.push_back(c);
		++i;
	}
end:
	return buf;
}

bool Tokenizer::getConstStr(StringRef data, char &quote_type, size_t &i, size_t &line,
			    size_t &line_start, String &buf)
{
	size_t len = data.size();
	buf.clear();
	quote_type		    = CURR;
	int starting_at		    = i;
	size_t continuous_backslash = 0;
	// omit beginning quote
	++i;
	while(i < len) {
		if(CURR == '\n') {
			++line;
			line_start = i + 1;
		}
		if(CURR == '\\') {
			++continuous_backslash;
			buf.push_back(data[i++]);
			continue;
		}
		if(CURR == quote_type && continuous_backslash % 2 == 0) break;
		buf.push_back(data[i++]);
		if(quote_type == '\'') {
			if(CURR != quote_type) {
				err::out(
				loc(line, starting_at - line_start),
				"expected single quote for end of const char, found: ", CURR);
				return false;
			}
			break;
		}
		continuous_backslash = 0;
	}
	if(CURR != quote_type) {
		err::out(loc(line, starting_at - line_start), "no matching quote for '", quote_type,
			 "' found");
		return false;
	}
	// omit ending quote
	++i;
	removeBackSlash(buf);
	return true;
}

TokType Tokenizer::getOperator(StringRef data, size_t &i, size_t line, size_t line_start)
{
	size_t len	   = data.size();
	TokType op_type	   = INVALID;
	size_t starting_at = i;
	switch(CURR) {
	case '+':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(ADD_ASSN);
			}
			if(NEXT == '+') {
				++i;
				SET_OP_TYPE_BRK(XINC);
			}
		}
		SET_OP_TYPE_BRK(ADD);
	case '-':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(SUB_ASSN);
			}
			if(NEXT == '-') {
				++i;
				SET_OP_TYPE_BRK(XDEC);
			}
			if(NEXT == '>') {
				++i;
				SET_OP_TYPE_BRK(ARROW);
			}
		}
		SET_OP_TYPE_BRK(SUB);
	case '*':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				if(CURR == '=') op_type = MUL_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK(MUL);
	case '/':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				if(CURR == '=') op_type = DIV_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK(DIV);
	case '%':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(MOD_ASSN);
			}
		}
		SET_OP_TYPE_BRK(MOD);
	case '&':
		if(i < len - 1) {
			if(NEXT == '&' || NEXT == '=') {
				++i;
				if(CURR == '&') op_type = LAND;
				else if(CURR == '=') op_type = BAND_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK(BAND);
	case '|':
		if(i < len - 1) {
			if(NEXT == '|' || NEXT == '=') {
				++i;
				if(CURR == '|') op_type = LOR;
				else if(CURR == '=') op_type = BOR_ASSN;
				break;
			}
		}
		SET_OP_TYPE_BRK(BOR);
	case '~':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(BNOT_ASSN);
			}
		}
		SET_OP_TYPE_BRK(BNOT);
	case '=':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(EQ);
			}
		}
		SET_OP_TYPE_BRK(ASSN);
	case '<':
		if(i < len - 1) {
			if(NEXT == '=' || NEXT == '<') {
				++i;
				if(CURR == '=') op_type = LE;
				else if(CURR == '<') {
					if(i < len - 1) {
						if(NEXT == '=') {
							++i;
							SET_OP_TYPE_BRK(LSHIFT_ASSN);
						}
					}
					op_type = LSHIFT;
				}
				break;
			}
		}
		SET_OP_TYPE_BRK(LT);
	case '>':
		if(i < len - 1) {
			if(NEXT == '=' || NEXT == '>') {
				++i;
				if(CURR == '=') op_type = GE;
				else if(CURR == '>') {
					if(i < len - 1) {
						if(NEXT == '=') {
							++i;
							SET_OP_TYPE_BRK(RSHIFT_ASSN);
						}
					}
					op_type = RSHIFT;
				}
				break;
			}
		}
		SET_OP_TYPE_BRK(GT);
	case '!':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(NE);
			}
		}
		SET_OP_TYPE_BRK(LNOT);
	case '^':
		if(i < len - 1) {
			if(NEXT == '=') {
				++i;
				SET_OP_TYPE_BRK(BXOR_ASSN);
			}
		}
		SET_OP_TYPE_BRK(BXOR);
	case ' ': SET_OP_TYPE_BRK(SPC);
	case '\t': SET_OP_TYPE_BRK(TAB);
	case '\n': SET_OP_TYPE_BRK(NEWL);
	case '.':
		if(i < len - 1 && NEXT == '.') {
			++i;
			if(i < len - 1 && NEXT == '.') {
				++i;
				SET_OP_TYPE_BRK(PreVA);
			}
		}
		SET_OP_TYPE_BRK(DOT);
	case '?': SET_OP_TYPE_BRK(QUEST);
	case ':': SET_OP_TYPE_BRK(COL);
	case ',': SET_OP_TYPE_BRK(COMMA);
	case ';': SET_OP_TYPE_BRK(COLS);
	case '@': SET_OP_TYPE_BRK(AT);
	case '(': SET_OP_TYPE_BRK(LPAREN);
	case '[': SET_OP_TYPE_BRK(LBRACK);
	case '{': SET_OP_TYPE_BRK(LBRACE);
	case ')': SET_OP_TYPE_BRK(RPAREN);
	case ']': SET_OP_TYPE_BRK(RBRACK);
	case '}': SET_OP_TYPE_BRK(RBRACE);
	default:
		err::out(loc(line, starting_at - line_start), "unknown operator '", CURR,
			 "' found");
		op_type = INVALID;
	}

	++i;
	return op_type;
}

void Tokenizer::removeBackSlash(String &s)
{
	for(auto it = s.begin(); it != s.end(); ++it) {
		if(*it == '\\') {
			if(it + 1 >= s.end()) continue;
			it = s.erase(it);
			if(*it == '0') *it = '\0';
			else if(*it == 'a') *it = '\a';
			else if(*it == 'b') *it = '\b';
#if !defined(OS_WINDOWS)
			else if(*it == 'e') *it = '\e';
#endif
			else if(*it == 'f') *it = '\f';
			else if(*it == 'n') *it = '\n';
			else if(*it == 'r') *it = '\r';
			else if(*it == 't') *it = '\t';
			else if(*it == 'v') *it = '\v';
		}
	}
}

String viewBackSlash(StringRef data)
{
	String res(data);
	for(auto it = res.begin(); it != res.end(); ++it) {
		if(*it == '\0') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\0");
			continue;
		}
		if(*it == '\a') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\a");
			continue;
		}
		if(*it == '\b') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\b");
			continue;
		}
#if !defined(OS_WINDOWS)
		if(*it == '\e') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\e");
			continue;
		}
#endif
		if(*it == '\f') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\f");
			continue;
		}
		if(*it == '\n') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\n");
			continue;
		}
		if(*it == '\r') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\r");
			continue;
		}
		if(*it == '\t') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\t");
			continue;
		}
		if(*it == '\v') {
			it = res.erase(it);
			res.insert(it - res.begin(), "\\v");
			continue;
		}
	}
	return res;
}
} // namespace lex
} // namespace sc