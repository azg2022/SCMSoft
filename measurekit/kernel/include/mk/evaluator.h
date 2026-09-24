#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "mk/ast.h"

namespace mk {

// 变量绑定环境；内置常量 pi 和 e 无需放入 env
using Env = std::unordered_map<std::string, double>;

// 分步求值记录：一次求值过程中的一个中间步骤
struct EvalStep {
    std::string expr;  // 本步计算的子表达式原文
    double value;      // 该子表达式的计算结果
};

// 对 AST 求值；定义域错误/未定义变量/未知函数等抛 MkError（中文消息）
double evaluate(const Node& n, const Env& env);

// 与 evaluate 相同，但把每个节点的归约按后序（先子后父）追加到 steps。
// 纯数字字面量不产生步骤；求值中途抛错时 steps 保留已完成的步骤。
// steps 传 nullptr 时行为与 evaluate 完全一致。
double evaluate(const Node& n, const Env& env, std::vector<EvalStep>* steps);

} // namespace mk
