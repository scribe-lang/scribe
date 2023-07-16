#include "Error.hpp"

#include <iostream>

#include "Parser.hpp"

namespace sc
{
ModuleLoc::ModuleLoc(Module *mod, size_t line, size_t col) : mod(mod), line(line), col(col) {}

String ModuleLoc::getLocStr() const
{
	return std::to_string(line + 1) + ":" + std::to_string(col + 1);
}

namespace err
{

static size_t max_errs = 10;

void setMaxErrs(size_t max_err) { max_errs = max_err; }

void outCommon(const ModuleLoc &loc, InitList<StringRef> err, bool is_warn);

void out(Stmt *stmt, InitList<StringRef> err) { out(stmt->getLoc(), err); }
void out(const lex::Lexeme &tok, InitList<StringRef> err) { out(*tok.getLoc(), err); }
void out(const ModuleLoc &loc, InitList<StringRef> err) { outCommon(loc, err, false); }

// equivalent to out(), but for warnings
void outw(Stmt *stmt, InitList<StringRef> err) { outw(stmt->getLoc(), err); }
void outw(const lex::Lexeme &tok, InitList<StringRef> err) { outw(*tok.getLoc(), err); }
void outw(const ModuleLoc &loc, InitList<StringRef> err) { outCommon(loc, err, true); }

void outCommon(const ModuleLoc &loc, InitList<StringRef> err, bool is_warn)
{
	static size_t errcount = 0;

	if(errcount >= max_errs) return;

	Module *mod = loc.getMod();
	size_t line = loc.getLine();
	size_t col  = loc.getCol();

	size_t linectr = 0;
	size_t idx     = 0;
	bool found     = false;

	StringRef data	   = mod->getCode();
	StringRef filename = mod->getPath();

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
	StringRef err_line = "<not found>";
	if(found) {
		size_t count = data.find('\n', idx);
		if(count != String::npos) count -= idx;
		err_line = data.substr(idx, count);
	}

	size_t tab_count = 0;
	for(auto &c : err_line) {
		if(c == '\t') ++tab_count;
	}
	String spacing_caret(col /* + 1 for single character '^' */, ' ');
	while(tab_count--) {
		spacing_caret.pop_back();
		spacing_caret.insert(spacing_caret.begin(), '\t');
	}

	std::cout << filename << " (" << line + 1 << ":" << col + 1
		  << "): " << (is_warn ? "Warning" : "Failure") << ": ";
	for(auto &e : err) std::cout << e;
	std::cout << "\n";
	std::cout << err_line << "\n";
	std::cout << spacing_caret << "^\n";

	if(!is_warn) ++errcount;
	if(errcount >= max_errs) {
		std::cout << "Failure: Too many errors encountered\n";
	}
}

} // namespace err
} // namespace sc