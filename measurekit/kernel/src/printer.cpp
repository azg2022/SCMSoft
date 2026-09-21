#include "mk/printer.h"

#include <cstdio>

namespace mk {

namespace {

// 优先级：原子(Number/Variable/Call) 4，'^' 3，'*''/' 2，'+''-' 1，一元负号视为 1
int precedence(const Node& n) {
    switch (n.kind) {
    case Node::Kind::Number:
    case Node::Kind::Variable:
    case Node::Kind::Call:
        return 4;
    case Node::Kind::Unary:
        if (n.op == '!' || n.op == '%')
            return 4;
        return 1; // 一元 '-','+'
    case Node::Kind::Binary:
        switch (n.op) {
        case '^': return 3;
        case '*': case '/': return 2;
        default: return 1;
        }
    }
    return 0;
}

bool isAtomic(const Node& n) {
    return n.kind == Node::Kind::Number || n.kind == Node::Kind::Variable ||
           n.kind == Node::Kind::Call;
}

std::string formatNumber(double v) {
    // %g 风格去掉多余尾随零（2 而非 2.000000），12 位有效数字兼顾精度
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.12g", v);
    return buf;
}

std::string render(const Node& n) {
    switch (n.kind) {
    case Node::Kind::Number:
        return formatNumber(n.num);
    case Node::Kind::Variable:
        return n.name;
    case Node::Kind::Call: {
        std::string s = n.name + "(";
        for (std::size_t i = 0; i < n.args.size(); ++i) {
            if (i) s += ",";
            s += render(*n.args[i]);
        }
        s += ")";
        return s;
    }
    case Node::Kind::Unary: {
        const Node& child = *n.args[0];
        if (n.op == '!' || n.op == '%') {
            std::string s = render(child);
            if (!isAtomic(child) &&
                !(child.kind == Node::Kind::Unary && (child.op == '!' || child.op == '%')))
                s = "(" + s + ")";
            return s + n.op;
        }
        // 一元 '-','+'：子节点是二元运算时需加括号
        std::string s = render(child);
        if (child.kind == Node::Kind::Binary)
            s = "(" + s + ")";
        return std::string(1, n.op) + s;
    }
    case Node::Kind::Binary: {
        int self = precedence(n);
        const Node& l = *n.args[0];
        const Node& r = *n.args[1];
        std::string ls = render(l);
        std::string rs = render(r);

        if (n.op == '^') {
            // 幂的底数是二元运算或一元负号时必须加括号：(-2)^2、(a+b)^2
            if (l.kind == Node::Kind::Binary ||
                (l.kind == Node::Kind::Unary && (l.op == '-' || l.op == '+')))
                ls = "(" + ls + ")";
        } else if (precedence(l) < self) {
            ls = "(" + ls + ")";
        }

        // 右操作数优先级小于等于自身时加括号，保证 a-(b-c)、a/(b/c) 不被打错
        if (precedence(r) <= self)
            rs = "(" + rs + ")";

        return ls + n.op + rs;
    }
    }
    return "";
}

} // namespace

std::string toString(const Node& n) {
    return render(n);
}

} // namespace mk
