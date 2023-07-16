#include "CodeGen/Base.hpp"

#include "Parser.hpp"

namespace sc
{
CodeGenDriver::CodeGenDriver(RAIIParser &parser) : ctx(parser.getContext()), parser(parser) {}
CodeGenDriver::~CodeGenDriver() {}
} // namespace sc