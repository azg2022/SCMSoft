#include "mk/ast.h"

namespace mk {

NodePtr num(double v) {
    auto n = std::make_unique<Node>();
    n->kind = Node::Kind::Number;
    n->num = v;
    return n;
}

NodePtr var(const std::string& name) {
    auto n = std::make_unique<Node>();
    n->kind = Node::Kind::Variable;
    n->name = name;
    return n;
}

NodePtr unary(char op, NodePtr a) {
    auto n = std::make_unique<Node>();
    n->kind = Node::Kind::Unary;
    n->op = op;
    n->args.push_back(std::move(a));
    return n;
}

NodePtr binary(char op, NodePtr l, NodePtr r) {
    auto n = std::make_unique<Node>();
    n->kind = Node::Kind::Binary;
    n->op = op;
    n->args.push_back(std::move(l));
    n->args.push_back(std::move(r));
    return n;
}

NodePtr call(const std::string& fn, std::vector<NodePtr> args) {
    auto n = std::make_unique<Node>();
    n->kind = Node::Kind::Call;
    n->name = fn;
    n->args = std::move(args);
    return n;
}

NodePtr call1(const std::string& fn, NodePtr a) {
    std::vector<NodePtr> args;
    args.push_back(std::move(a));
    return call(fn, std::move(args));
}

NodePtr clone(const Node& n) {
    auto c = std::make_unique<Node>();
    c->kind = n.kind;
    c->num = n.num;
    c->name = n.name;
    c->op = n.op;
    c->args.reserve(n.args.size());
    for (const auto& a : n.args)
        c->args.push_back(clone(*a));
    return c;
}

} // namespace mk
