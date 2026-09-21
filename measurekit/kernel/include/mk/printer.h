#pragma once

#include <string>

#include "mk/ast.h"

namespace mk {

// AST → 可读字符串，按优先级加最少的括号
std::string toString(const Node& n);

} // namespace mk
