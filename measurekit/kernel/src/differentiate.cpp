#include "mk/differentiate.h"

#include "mk/error.h"
#include "mk/simplify.h"

namespace mk {

namespace {

// 外层函数导数（链式法则的外层因子），a 为内层表达式的克隆
NodePtr outerDeriv(const std::string& fn, NodePtr a) {
    if (fn == "sin") return call1("cos", std::move(a));
    if (fn == "cos") return unary('-', call1("sin", std::move(a)));
    if (fn == "tan") return binary('/', num(1), binary('^', call1("cos", std::move(a)), num(2)));
    if (fn == "exp") return call1("exp", std::move(a));
    if (fn == "ln") return binary('/', num(1), std::move(a));
    if (fn == "sqrt")
        return binary('/', num(1), binary('*', num(2), call1("sqrt", std::move(a))));
    if (fn == "asin")
        return binary('/', num(1),
                      call1("sqrt", binary('-', num(1), binary('^', std::move(a), num(2)))));
    if (fn == "acos")
        return unary('-', binary('/', num(1),
                                 call1("sqrt", binary('-', num(1),
                                                      binary('^', std::move(a), num(2))))));
    if (fn == "atan")
        return binary('/', num(1), binary('+', num(1), binary('^', std::move(a), num(2))));
    if (fn == "sinh") return call1("cosh", std::move(a));
    if (fn == "cosh") return call1("sinh", std::move(a));
    if (fn == "tanh")
        return binary('/', num(1), binary('^', call1("cosh", std::move(a)), num(2)));
    throw MkError("函数 '" + fn + "' 暂不支持符号求导");
}

NodePtr diff(const Node& n, const std::string& var) {
    switch (n.kind) {
    case Node::Kind::Number:
        return num(0);
    case Node::Kind::Variable:
        return num(n.name == var ? 1 : 0); // 其他变量视为常数（偏导语义）
    case Node::Kind::Unary: {
        const Node& c = *n.args[0];
        switch (n.op) {
        case '-': return unary('-', diff(c, var));
        case '+': return diff(c, var);
        case '!':
            if (c.kind == Node::Kind::Number)
                return num(0); // 常数阶乘导数为 0
            throw MkError("阶乘不支持求导");
        case '%': // x% 视为 x/100
            return binary('/', diff(c, var), num(100));
        }
        throw MkError("未知的一元运算符");
    }
    case Node::Kind::Binary: {
        const Node& l = *n.args[0];
        const Node& r = *n.args[1];
        switch (n.op) {
        case '+': return binary('+', diff(l, var), diff(r, var));
        case '-': return binary('-', diff(l, var), diff(r, var));
        case '*': // 乘法法则：(l'r + lr')
            return binary('+',
                          binary('*', diff(l, var), clone(r)),
                          binary('*', clone(l), diff(r, var)));
        case '/': // 商的法则：(l'r - lr') / r^2
            return binary('/',
                          binary('-',
                                 binary('*', diff(l, var), clone(r)),
                                 binary('*', clone(l), diff(r, var))),
                          binary('^', clone(r), num(2)));
        case '^':
            if (r.kind == Node::Kind::Number) {
                // 幂法则：n*f^(n-1)*f'
                double c = r.num;
                return binary('*',
                              binary('*', num(c), binary('^', clone(l), num(c - 1))),
                              diff(l, var));
            }
            // 一般情形：f^g * (g'*ln(f) + g*f'/f)
            return binary('*',
                          clone(n),
                          binary('+',
                                 binary('*', diff(r, var), call1("ln", clone(l))),
                                 binary('/', binary('*', clone(r), diff(l, var)), clone(l))));
        }
        throw MkError("未知的二元运算符");
    }
    case Node::Kind::Call:
        if (n.args.size() != 1)
            throw MkError("函数 '" + n.name + "' 暂不支持符号求导");
        // 链式法则：内层导数 * 外层导数
        return binary('*', diff(*n.args[0], var), outerDeriv(n.name, clone(*n.args[0])));
    }
    throw MkError("无法对该节点求导");
}

} // namespace

NodePtr differentiate(const Node& n, const std::string& var) {
    return simplify(diff(n, var));
}

NodePtr differentiate(const Node& n, const std::string& var, unsigned order) {
    if (order == 0)
        throw MkError("求导阶数必须 ≥ 1");
    NodePtr cur = differentiate(n, var);
    for (unsigned i = 1; i < order; ++i)
        cur = differentiate(*cur, var);
    return cur;
}

} // namespace mk
