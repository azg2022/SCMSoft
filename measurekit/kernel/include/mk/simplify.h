#pragma once

#include "mk/ast.h"

namespace mk {

// 表达式化简：自底向上反复应用规则直到不动点（内部设迭代上限防死循环）。
// 规则包括常数折叠、零/幺元消除、双重负号、同类项合并等，详见实现。
NodePtr simplify(NodePtr n);

} // namespace mk
