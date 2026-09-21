#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mk {

struct Node;
using NodePtr = std::unique_ptr<Node>;

// 表达式 AST 节点。后续符号微分、化简均基于该结构。
// 采用单一结构体 + Kind 判别，避免继承体系，便于遍历与克隆。
struct Node {
    enum class Kind { Number, Variable, Unary, Binary, Call } kind;

    double num = 0.0;      // Kind == Number
    std::string name;      // Kind == Variable 的变量名 / Kind == Call 的函数名
    char op = 0;           // Unary: '-','+','!','%' ; Binary: '+','-','*','/','^'
    std::vector<NodePtr> args; // Unary 1 个, Binary 2 个, Call n 个
};

NodePtr num(double v);
NodePtr var(const std::string& name);
NodePtr unary(char op, NodePtr a);
NodePtr binary(char op, NodePtr l, NodePtr r);
NodePtr call(const std::string& fn, std::vector<NodePtr> args);
NodePtr call1(const std::string& fn, NodePtr a); // 单参数便捷构造
NodePtr clone(const Node& n); // 深拷贝

} // namespace mk
