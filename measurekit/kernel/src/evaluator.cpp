#include "mk/evaluator.h"

#include <cmath>

#include "mk/error.h"

namespace mk {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kE = 2.71828182845904523536;

bool isIntegral(double x) {
    return std::isfinite(x) && std::floor(x) == x;
}

double factorial(double x) {
    if (isIntegral(x)) {
        if (x <= -1)
            throw MkError("阶乘要求非负整数或 x > -1");
        // 非负整数直接累乘，结果精确
        double r = 1.0;
        for (double k = 2.0; k <= x; k += 1.0)
            r *= k;
        return r;
    }
    // 非整数用伽马函数：x! = Γ(x+1)
    return std::tgamma(x + 1.0);
}

// 单参数初等函数表；log 单独处理以支持双参数形式
double applyUnaryFn(const std::string& fn, double x) {
    if (fn == "sin") return std::sin(x);
    if (fn == "cos") return std::cos(x);
    if (fn == "tan") {
        if (std::fabs(std::cos(x)) < 1e-15)
            throw MkError("正切在该点无定义");
        return std::tan(x);
    }
    if (fn == "asin") {
        if (std::fabs(x) > 1.0)
            throw MkError("反正弦/反余弦要求 |x| ≤ 1");
        return std::asin(x);
    }
    if (fn == "acos") {
        if (std::fabs(x) > 1.0)
            throw MkError("反正弦/反余弦要求 |x| ≤ 1");
        return std::acos(x);
    }
    if (fn == "atan") return std::atan(x);
    if (fn == "sinh") return std::sinh(x);
    if (fn == "cosh") return std::cosh(x);
    if (fn == "tanh") return std::tanh(x);
    if (fn == "exp") return std::exp(x);
    if (fn == "ln") {
        if (x <= 0.0)
            throw MkError("对数的真数必须大于 0");
        return std::log(x);
    }
    if (fn == "log") {
        if (x <= 0.0)
            throw MkError("对数的真数必须大于 0");
        return std::log10(x);
    }
    if (fn == "log2") {
        if (x <= 0.0)
            throw MkError("对数的真数必须大于 0");
        return std::log2(x);
    }
    if (fn == "sqrt") {
        if (x < 0.0)
            throw MkError("√x 要求 x ≥ 0");
        return std::sqrt(x);
    }
    if (fn == "cbrt") return std::cbrt(x);
    if (fn == "abs") return std::fabs(x);
    if (fn == "floor") return std::floor(x);
    if (fn == "ceil") return std::ceil(x);
    if (fn == "round") return std::round(x);
    if (fn == "gamma") return std::tgamma(x);
    if (fn == "erf") return std::erf(x);
    return std::numeric_limits<double>::quiet_NaN(); // 不会到达：调用前已校验函数名
}

bool isKnownFn(const std::string& fn) {
    static const char* kFns[] = {
        "sin", "cos", "tan", "asin", "acos", "atan",
        "sinh", "cosh", "tanh", "exp", "ln", "log", "log2",
        "sqrt", "cbrt", "abs", "floor", "ceil", "round", "gamma", "erf",
    };
    for (const char* f : kFns)
        if (fn == f)
            return true;
    return false;
}

double evalNode(const Node& n, const Env& env) {
    double result = 0.0;

    switch (n.kind) {
    case Node::Kind::Number:
        result = n.num;
        break;

    case Node::Kind::Variable:
        if (auto it = env.find(n.name); it != env.end())
            result = it->second;
        else if (n.name == "pi")
            result = kPi;
        else if (n.name == "e")
            result = kE;
        else
            throw MkError("未定义的变量 '" + n.name + "'");
        break;

    case Node::Kind::Unary: {
        double a = evalNode(*n.args[0], env);
        switch (n.op) {
        case '-': result = -a; break;
        case '+': result = a; break;
        case '!': result = factorial(a); break;
        case '%': result = a / 100.0; break;
        default:
            throw MkError(std::string("未知的一元运算符 '") + n.op + "'");
        }
        break;
    }

    case Node::Kind::Binary: {
        double l = evalNode(*n.args[0], env);
        double r = evalNode(*n.args[1], env);
        switch (n.op) {
        case '+': result = l + r; break;
        case '-': result = l - r; break;
        case '*': result = l * r; break;
        case '/':
            if (r == 0.0)
                throw MkError("除数不能为零");
            result = l / r;
            break;
        case '^': result = std::pow(l, r); break;
        default:
            throw MkError(std::string("未知的二元运算符 '") + n.op + "'");
        }
        break;
    }

    case Node::Kind::Call: {
        if (!isKnownFn(n.name))
            throw MkError("未知函数 '" + n.name + "'");
        if (n.name == "log" && n.args.size() == 2) {
            double base = evalNode(*n.args[0], env);
            double x = evalNode(*n.args[1], env);
            if (base <= 0.0 || base == 1.0)
                throw MkError("对数的底数必须大于 0 且不等于 1");
            if (x <= 0.0)
                throw MkError("对数的真数必须大于 0");
            result = std::log(x) / std::log(base);
            break;
        }
        if (n.args.size() != 1)
            throw MkError("函数 '" + n.name + "' 需要 1 个参数，实际给了 " +
                          std::to_string(n.args.size()) + " 个");
        result = applyUnaryFn(n.name, evalNode(*n.args[0], env));
        break;
    }
    }

    if (!std::isfinite(result))
        throw MkError("数值溢出或结果超出范围");
    return result;
}

} // namespace

double evaluate(const Node& n, const Env& env) {
    return evalNode(n, env);
}

} // namespace mk
