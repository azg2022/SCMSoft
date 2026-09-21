#include "mk/simplify.h"

#include <utility>

#include "mk/error.h"
#include "mk/evaluator.h"

namespace mk {

namespace {

bool isNum(const Node& n, double v) {
    return n.kind == Node::Kind::Number && n.num == v;
}

bool nodeEqual(const Node& a, const Node& b) {
    if (a.kind != b.kind || a.op != b.op || a.name != b.name)
        return false;
    if (a.kind == Node::Kind::Number && a.num != b.num)
        return false;
    if (a.args.size() != b.args.size())
        return false;
    for (std::size_t i = 0; i < a.args.size(); ++i)
        if (!nodeEqual(*a.args[i], *b.args[i]))
            return false;
    return true;
}

bool allNumbers(const Node& n) {
    if (n.kind == Node::Kind::Number)
        return true;
    if (n.kind == Node::Kind::Variable)
        return false;
    for (const auto& a : n.args)
        if (!allNumbers(*a))
            return false;
    return true;
}

// 把项拆成 "系数 * 余项"：c*t 或 t*c（c 为 Number），否则系数视为 1
std::pair<double, const Node*> splitCoef(const Node& n) {
    if (n.kind == Node::Kind::Binary && n.op == '*') {
        if (n.args[0]->kind == Node::Kind::Number)
            return {n.args[0]->num, n.args[1].get()};
        if (n.args[1]->kind == Node::Kind::Number)
            return {n.args[1]->num, n.args[0].get()};
    }
    return {1.0, &n};
}

NodePtr applyLocal(NodePtr n, bool& changed);

NodePtr simplifyRec(NodePtr n, bool& changed) {
    for (auto& a : n->args)
        a = simplifyRec(std::move(a), changed);
    return applyLocal(std::move(n), changed);
}

NodePtr applyLocal(NodePtr n, bool& changed) {
    // 常数折叠：子树全为 Number 时直接求值替换；求值失败（如 0/0）保留原样
    if (n->kind == Node::Kind::Unary || n->kind == Node::Kind::Binary ||
        n->kind == Node::Kind::Call) {
        if (allNumbers(*n)) {
            try {
                double v = evaluate(*n, {});
                changed = true;
                return num(v);
            } catch (const MkError&) {
            }
        }
    }

    if (n->kind == Node::Kind::Unary) {
        Node& c = *n->args[0];
        if (n->op == '-' && c.kind == Node::Kind::Unary && c.op == '-') {
            changed = true; // -(-x) → x
            return std::move(c.args[0]);
        }
        if (n->op == '%') { // x% → x/100，便于求导与后续化简
            changed = true;
            return binary('/', std::move(n->args[0]), num(100.0));
        }
        if (n->op == '+') { // 一元正号透传（解析期已不产生，防御性处理）
            changed = true;
            return std::move(n->args[0]);
        }
        return n;
    }

    if (n->kind != Node::Kind::Binary)
        return n;

    NodePtr& l = n->args[0];
    NodePtr& r = n->args[1];
    switch (n->op) {
    case '+': {
        if (isNum(*r, 0)) { changed = true; return std::move(l); }
        if (isNum(*l, 0)) { changed = true; return std::move(r); }
        // a*x + b*x → (a+b)*x（含 x+x → 2*x）
        auto [ca, ta] = splitCoef(*l);
        auto [cb, tb] = splitCoef(*r);
        if (nodeEqual(*ta, *tb)) {
            changed = true;
            return binary('*', num(ca + cb), clone(*ta));
        }
        return n;
    }
    case '-':
        if (isNum(*r, 0)) { changed = true; return std::move(l); }
        if (isNum(*l, 0)) { changed = true; return unary('-', std::move(r)); } // 0-x → -x
        return n;
    case '*':
        if (isNum(*l, 0) || isNum(*r, 0)) { changed = true; return num(0); }
        if (isNum(*l, 1)) { changed = true; return std::move(r); }
        if (isNum(*r, 1)) { changed = true; return std::move(l); }
        // 常数系数合并：a*(b*x) → (a*b)*x；(a*x)*b → (a*b)*x
        if (l->kind == Node::Kind::Number && r->kind == Node::Kind::Binary && r->op == '*') {
            if (r->args[0]->kind == Node::Kind::Number) {
                changed = true;
                return binary('*', num(l->num * r->args[0]->num), std::move(r->args[1]));
            }
            if (r->args[1]->kind == Node::Kind::Number) {
                changed = true;
                return binary('*', num(l->num * r->args[1]->num), std::move(r->args[0]));
            }
        }
        if (r->kind == Node::Kind::Number && l->kind == Node::Kind::Binary && l->op == '*') {
            if (l->args[0]->kind == Node::Kind::Number) {
                changed = true;
                return binary('*', num(l->args[0]->num * r->num), std::move(l->args[1]));
            }
            if (l->args[1]->kind == Node::Kind::Number) {
                changed = true;
                return binary('*', num(l->args[1]->num * r->num), std::move(l->args[0]));
            }
        }
        return n;
    case '/':
        // 右操作数为 Number 时（含 0/0）已由常数折叠或异常保留处理，此处跳过防止 0/0→0
        if (isNum(*l, 0) && r->kind != Node::Kind::Number) { changed = true; return num(0); }
        if (isNum(*r, 1)) { changed = true; return std::move(l); }
        return n;
    case '^':
        if (isNum(*r, 1)) { changed = true; return std::move(l); }
        if (isNum(*r, 0)) { changed = true; return num(1); }
        if (isNum(*l, 1)) { changed = true; return num(1); }
        if (isNum(*l, 0) && r->kind != Node::Kind::Number) { changed = true; return num(0); }
        return n;
    }
    return n;
}

} // namespace

NodePtr simplify(NodePtr n) {
    constexpr int kMaxIter = 100; // 迭代上限，防规则互相震荡导致死循环
    for (int i = 0; i < kMaxIter; ++i) {
        bool changed = false;
        n = simplifyRec(std::move(n), changed);
        if (!changed)
            break;
    }
    return n;
}

} // namespace mk
