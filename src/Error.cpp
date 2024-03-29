#include "Error.hpp"

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

size_t max_errs = 10;

void outCommonStr(const ModuleLoc *loc, bool iswarn, bool withloc, const String &e)
{
	static size_t errcount = 0;

	if(errcount >= max_errs) return;

	// just show the error
	if(!withloc) {
		std::cout << (iswarn ? "Warning" : "Failure") << ": ";
		std::cout << e;
		std::cout << "\n";
		if(!iswarn) ++errcount;
		if(errcount >= max_errs) std::cout << "Failure: Too many errors encountered\n";
		return;
	}

	Module *mod = loc->getMod();
	size_t line = loc->getLine();
	size_t col  = loc->getCol();

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
	String spacing_caret(col, ' ');
	while(tab_count--) {
		spacing_caret.pop_back();
		spacing_caret.insert(spacing_caret.begin(), '\t');
	}

	std::cout << filename << " (" << line + 1 << ":" << col + 1 << "): ";
	std::cout << (iswarn ? "Warning" : "Failure") << ": ";
	std::cout << e;
	std::cout << "\n";
	std::cout << err_line << "\n";
	std::cout << spacing_caret << "^\n";

	if(!iswarn) ++errcount;
	if(errcount >= max_errs) std::cout << "Failure: Too many errors encountered\n";
}

} // namespace err
} // namespace sc