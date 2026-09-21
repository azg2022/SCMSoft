#include <gtest/gtest.h>

#include <cmath>

#include "mk/differentiate.h"
#include "mk/error.h"
#include "mk/evaluator.h"
#include "mk/parser.h"
#include "mk/printer.h"

using namespace mk;

static std::string diffStr(const std::string& s, const std::string& v = "x",
                           unsigned order = 1) {
    NodePtr ast = parse(s);
    NodePtr d = order == 1 ? differentiate(*ast, v) : differentiate(*ast, v, order);
    return toString(*d);
}

TEST(Differentiate, ConstantsAndVariables) {
    EXPECT_EQ(diffStr("5"), "0");
    EXPECT_EQ(diffStr("x"), "1");
    EXPECT_EQ(diffStr("y"), "0"); // 偏导语义：其他变量视为常数
}

TEST(Differentiate, PowerRule) {
    EXPECT_EQ(diffStr("x^3"), "3*x^2");
    EXPECT_EQ(diffStr("x^2"), "2*x");
}

TEST(Differentiate, Trig) {
    EXPECT_EQ(diffStr("sin(x)"), "cos(x)");
    EXPECT_EQ(diffStr("cos(x)"), "-sin(x)");
}

TEST(Differentiate, ProductRule) {
    EXPECT_EQ(diffStr("x*sin(x)"), "sin(x)+x*cos(x)");
}

TEST(Differentiate, ChainRule) {
    EXPECT_EQ(diffStr("exp(2x)"), "2*exp(2*x)");
    EXPECT_EQ(diffStr("ln(x)"), "1/x");
}

TEST(Differentiate, PartialSemantics) {
    EXPECT_EQ(diffStr("x^2+y"), "2*x");
    EXPECT_EQ(diffStr("x^2+y", "y"), "1");
}

TEST(Differentiate, HigherOrder) {
    EXPECT_EQ(diffStr("x^4", "x", 2), "12*x^2");
    EXPECT_EQ(diffStr("x^4", "x", 3), "24*x");
    EXPECT_THROW(diffStr("x", "x", 0), MkError); // 阶数必须 ≥ 1
}

TEST(Differentiate, QuotientRule) {
    EXPECT_EQ(diffStr("1/x"), "-1/x^2");
}

TEST(Differentiate, Percent) {
    EXPECT_EQ(diffStr("x%"), "0.01"); // x% = x/100
}

TEST(Differentiate, Unsupported) {
    EXPECT_THROW(diffStr("gamma(x)"), MkError);     // 函数暂不支持
    EXPECT_THROW(diffStr("(x+1)!"), MkError);       // 非常数阶乘不支持求导
    EXPECT_THROW(diffStr("log(2,x)"), MkError);     // 多参数 log 暂不支持
    EXPECT_NO_THROW(diffStr("3!"));                 // 常数阶乘导数为 0
}

TEST(Differentiate, NumericCheck) {
    // 数值兜底：解析导函数与中心差分比较，容差 1e-6
    const char* cases[] = {"x^3",  "sin(x)*x", "exp(2x)", "ln(x)", "sqrt(x)",
                           "x^2*sin(x)", "tan(x)", "x^x", "1/(1+x^2)", "2^x"};
    const double xs[] = {0.5, 1.0, 2.3};
    const double h = 1e-5;
    for (const char* s : cases) {
        NodePtr f = parse(s);
        NodePtr df = differentiate(*f, "x");
        for (double x : xs) {
            Env env{{"x", x}};
            double exact = evaluate(*df, env);
            Env ep{{"x", x + h}}, em{{"x", x - h}};
            double approx = (evaluate(*f, ep) - evaluate(*f, em)) / (2 * h);
            EXPECT_NEAR(exact, approx, 1e-6) << s << " at x=" << x;
        }
    }
}

TEST(Differentiate, NumericCheckAtHalf) {
    // 定义域受限的函数只在 x=0.5 处抽验
    const char* cases[] = {"asin(x)", "acos(x)", "atan(x)", "sinh(x)", "cosh(x)", "tanh(x)"};
    const double h = 1e-5;
    for (const char* s : cases) {
        NodePtr f = parse(s);
        NodePtr df = differentiate(*f, "x");
        double exact = evaluate(*df, {{"x", 0.5}});
        double approx =
            (evaluate(*f, {{"x", 0.5 + h}}) - evaluate(*f, {{"x", 0.5 - h}})) / (2 * h);
        EXPECT_NEAR(exact, approx, 1e-6) << s;
    }
}
