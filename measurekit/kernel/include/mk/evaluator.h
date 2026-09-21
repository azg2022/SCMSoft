#pragma once

#include <string>
#include <unordered_map>

#include "mk/ast.h"

namespace mk {

// 变量绑定环境；内置常量 pi 和 e 无需放入 env
using Env = std::unordered_map<std::string, double>;

// 对 AST 求值；定义域错误/未定义变量/未知函数等抛 MkError（中文消息）
double evaluate(const Node& n, const Env& env);

} // namespace mk
