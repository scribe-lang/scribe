#include "Error.hpp"

#include "FS.hpp"

namespace sc
{

static Vector<String> moduleList;
// Maps file path with file contents.
// Only set when errors/warnings are thrown so as to not read files over and over again.
static StringMap<String> moduleData;

ModuleLoc::ModuleLoc() : moduleId(-1), offset(0) {}
ModuleLoc::ModuleLoc(StringRef filePath, size_t offset)
	: moduleId(getOrAddModuleIdForPath(filePath)), offset(offset)
{}
ModuleLoc::ModuleLoc(size_t moduleId, size_t offset) : moduleId(moduleId), offset(offset) {}

size_t ModuleLoc::getOrAddModuleIdForPath(StringRef filePath)
{
	for(size_t i = 0; i < moduleList.size(); ++i) {
		if(moduleList[i] == filePath) return i;
	}
	moduleList.push_back(String(filePath));
	return moduleList.size() - 1;
}
const String &ModuleLoc::getModuleNameFromId(size_t id) { return moduleList[id]; }

namespace err
{

size_t max_errs = 10;

static bool getLineAndOffsetFromData(StringRef data, size_t offset, StringRef &line,
				     size_t &lineNum, size_t &lineOffset);

void outCommonStr(const ModuleLoc *loc, bool iswarn, bool withloc, const String &e)
{
	static size_t errcount = 0;
	// To prevent errors from showing up at the same location multiple times therefore
	// cluttering the output
	static const ModuleLoc *lastLoc = nullptr;

	if(errcount >= max_errs) return;

	if(lastLoc && lastLoc->getModuleId() == loc->getModuleId() &&
	   lastLoc->getOffset() == loc->getOffset())
		return;

	const String *path = nullptr;
	String spacingCaret;
	StringRef line;
	size_t offset, lineNum, lineOffset, tabCount = 0;

	// just show the error
	if(!withloc || !loc->isValid()) goto unknownLoc;

	lastLoc = loc;

	path   = &loc->getModuleName();
	offset = loc->getOffset();

	if(!moduleData.contains(*path)) {
		// load file
		String data;
		if(!fs::read(path->c_str(), data)) {
			std::cerr << "Failed to read file where error was located: " << path
				  << "\n";
			goto unknownLoc;
		}
		moduleData[*path] = std::move(data);
	}

	if(!getLineAndOffsetFromData(moduleData[*path], offset, line, lineNum, lineOffset)) {
		goto unknownLoc;
	}

	for(auto c : line) {
		if(c == '\t') ++tabCount;
	}
	spacingCaret = lineOffset > 0 ? String(lineOffset - 1, ' ') : "";
	while(tabCount--) {
		spacingCaret.pop_back();
		spacingCaret.insert(spacingCaret.begin(), '\t');
	}

	std::cerr << *path << " (" << lineNum << ":" << lineOffset << "): ";
unknownLoc:
	if(!withloc) std::cerr << "<Unknown location>: ";
	std::cerr << (iswarn ? "Warning" : "Failure") << ": ";
	std::cerr << e << "\n";
	if(!line.empty()) {
		std::cerr << line << "\n";
		std::cerr << spacingCaret << "^\n";
	}
end:
	if(!iswarn) ++errcount;
	if(!iswarn && errcount >= max_errs) std::cerr << "Failure: Too many errors encountered\n";
}

static bool getLineAndOffsetFromData(StringRef data, size_t offset, StringRef &line,
				     size_t &lineNum, size_t &lineOffset)
{
	line	   = "";
	lineNum	   = 1;
	lineOffset = -1;

	size_t lineStart = 0, lineEnd = 0;
	bool breakOnNewline = false;
	for(size_t i = 0; i < data.size(); ++i) {
		if(data[i] == '\n') {
			if(breakOnNewline) break;
			lineStart = i + 1;
			lineEnd	  = i;
			++lineNum;
			continue;
		}
		if(i == offset) {
			breakOnNewline = true;
			lineOffset     = i - lineStart;
		}
		++lineEnd;
	}
	if(lineOffset == -1) {
		lineNum = 0;
		return false;
	}
	line = StringRef(&data[lineStart], lineEnd - lineStart + 1);
	return true;
}

} // namespace err
} // namespace sc