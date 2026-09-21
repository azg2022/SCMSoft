#pragma once

#include <string>

#include "mk/ast.h"

namespace mk {

// 对 var 求一阶导（其他变量视为常数，即偏导语义），结果经化简器化简后返回
NodePtr differentiate(const Node& n, const std::string& var);

// 高阶导数（循环调用一阶），order >= 1
NodePtr differentiate(const Node& n, const std::string& var, unsigned order);

} // namespace mk
