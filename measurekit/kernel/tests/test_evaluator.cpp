#include <gtest/gtest.h>

#include <cmath>

#include "mk/error.h"
#include "mk/evaluator.h"
#include "mk/parser.h"

using namespace mk;

static double eval(const std::string& src, const Env& env = {}) {
    return evaluate(*parse(src), env);
}

TEST(Evaluator, Arithmetic) {
    EXPECT_DOUBLE_EQ(eval("1+2*3"), 7.0);
    EXPECT_DOUBLE_EQ(eval("10/4"), 2.5);
    EXPECT_DOUBLE_EQ(eval("2^10"), 1024.0);
    EXPECT_DOUBLE_EQ(eval("-3+5"), 2.0);
}

TEST(Evaluator, Factorial) {
    EXPECT_DOUBLE_EQ(eval("5!"), 120.0);
    EXPECT_DOUBLE_EQ(eval("3!"), 6.0);
    EXPECT_DOUBLE_EQ(eval("0!"), 1.0);
    EXPECT_NEAR(eval("0.5!"), std::tgamma(1.5), 1e-12); // 非整数走伽马函数
}

TEST(Evaluator, Percent) {
    EXPECT_DOUBLE_EQ(eval("50%"), 0.5);
    EXPECT_DOUBLE_EQ(eval("50%+1"), 1.5);
}

TEST(Evaluator, BuiltinConstants) {
    EXPECT_NEAR(eval("pi"), 3.141592653589793, 1e-15);
    EXPECT_NEAR(eval("e"), 2.718281828459045, 1e-15);
}

TEST(Evaluator, ElementaryFunctions) {
    EXPECT_NEAR(eval("sin(pi/2)"), 1.0, 1e-12);
    EXPECT_NEAR(eval("cos(0)"), 1.0, 1e-12);
    EXPECT_NEAR(eval("tan(pi/4)"), 1.0, 1e-12);
    EXPECT_NEAR(eval("asin(1)"), M_PI / 2, 1e-12);
    EXPECT_NEAR(eval("acos(1)"), 0.0, 1e-12);
    EXPECT_NEAR(eval("atan(1)"), M_PI / 4, 1e-12);
    EXPECT_NEAR(eval("sinh(1)"), std::sinh(1.0), 1e-12);
    EXPECT_NEAR(eval("cosh(1)"), std::cosh(1.0), 1e-12);
    EXPECT_NEAR(eval("tanh(1)"), std::tanh(1.0), 1e-12);
    EXPECT_NEAR(eval("exp(1)"), M_E, 1e-12);
    EXPECT_NEAR(eval("ln(e)"), 1.0, 1e-12);
    EXPECT_NEAR(eval("log(100)"), 2.0, 1e-12);
    EXPECT_NEAR(eval("log2(8)"), 3.0, 1e-12);
    EXPECT_NEAR(eval("sqrt(2)"), std::sqrt(2.0), 1e-12);
    EXPECT_NEAR(eval("cbrt(27)"), 3.0, 1e-12);
    EXPECT_DOUBLE_EQ(eval("abs(-3)"), 3.0);
    EXPECT_DOUBLE_EQ(eval("floor(2.7)"), 2.0);
    EXPECT_DOUBLE_EQ(eval("ceil(2.1)"), 3.0);
    EXPECT_DOUBLE_EQ(eval("round(2.5)"), 3.0);
    EXPECT_NEAR(eval("gamma(5)"), 24.0, 1e-12);
    EXPECT_NEAR(eval("erf(1)"), std::erf(1.0), 1e-12);
}

TEST(Evaluator, LogWithBase) {
    EXPECT_NEAR(eval("log(2,8)"), 3.0, 1e-12);
    EXPECT_NEAR(eval("log(10,1000)"), 3.0, 1e-12);
}

TEST(Evaluator, DomainErrors) {
    try {
        eval("1/0");
        FAIL();
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "除数不能为零");
    }
    try {
        eval("sqrt(-1)");
        FAIL();
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "√x 要求 x ≥ 0");
    }
    try {
        eval("ln(0)");
        FAIL();
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "对数的真数必须大于 0");
    }
    try {
        eval("asin(2)");
        FAIL();
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "反正弦/反余弦要求 |x| ≤ 1");
    }
    EXPECT_THROW(eval("tan(pi/2)"), MkError);
    EXPECT_THROW(eval("log(1,10)"), MkError);  // 底数为 1
    EXPECT_THROW(eval("log(-2,8)"), MkError);  // 底数为负
    EXPECT_THROW(eval("(-3)!"), MkError);      // 负整数阶乘
}

TEST(Evaluator, UndefinedVariable) {
    try {
        eval("xyz + 1");
        FAIL();
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "未定义的变量 'xyz'");
    }
}

TEST(Evaluator, UnknownFunction) {
    try {
        eval("foo(1)");
        FAIL();
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "未知函数 'foo'");
    }
}

TEST(Evaluator, WrongArgCount) {
    EXPECT_THROW(eval("sin(1,2)"), MkError);
}

TEST(Evaluator, Overflow) {
    EXPECT_THROW(eval("exp(1000)"), MkError);
}

TEST(Evaluator, EnvBinding) {
    Env env{{"x", 5.0}, {"y", 2.0}};
    EXPECT_DOUBLE_EQ(eval("x^2+2*x-1", env), 34.0);
    EXPECT_DOUBLE_EQ(eval("x*y", env), 10.0);
    // env 中的绑定优先于内置常量
    Env env2{{"pi", 3.0}};
    EXPECT_DOUBLE_EQ(eval("pi", env2), 3.0);
}
