#pragma once

#include <string>

#include "mk/ast.h"

namespace mk {

// 递归下降解析；语法错误抛 MkError（中文消息 + 字符位置）
NodePtr parse(const std::string& src);

} // namespace mk
