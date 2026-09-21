#include <gtest/gtest.h>

#include "mk/evaluator.h"
#include "mk/parser.h"
#include "mk/printer.h"

using namespace mk;

static std::string roundTrip(const std::string& s) {
    return toString(*parse(s));
}

TEST(Printer, BasicFormatting) {
    EXPECT_EQ(roundTrip("x+sin(x)"), "x+sin(x)");
    EXPECT_EQ(roundTrip("2.500000"), "2.5"); // %g 去掉尾随零
    EXPECT_EQ(roundTrip("1+2"), "1+2");
}

TEST(Printer, MinimalParens) {
    EXPECT_EQ(roundTrip("a-(b-c)"), "a-(b-c)");
    EXPECT_EQ(roundTrip("(a+b)*c"), "(a+b)*c");
    EXPECT_EQ(roundTrip("a+b*c"), "a+b*c");
    EXPECT_EQ(roundTrip("a/(b/c)"), "a/(b/c)");
}

TEST(Printer, Power) {
    // '^' 右结合，右操作数同优先级时补括号，重解析后 AST 等价
    EXPECT_EQ(roundTrip("2^3^2"), "2^(3^2)");
    EXPECT_EQ(roundTrip("(a+b)^2"), "(a+b)^2");
    EXPECT_EQ(roundTrip("2^-3"), "2^(-3)");
}

TEST(Printer, UnaryAndPostfix) {
    EXPECT_EQ(roundTrip("-x^2"), "-(x^2)");
    EXPECT_EQ(roundTrip("(x+1)!"), "(x+1)!");
    EXPECT_EQ(roundTrip("-(a+b)"), "-(a+b)");
    EXPECT_EQ(roundTrip("x%"), "x%");
}

TEST(Printer, Call) {
    EXPECT_EQ(roundTrip("log(2,8)"), "log(2,8)");
    EXPECT_EQ(roundTrip("sin(x+1)"), "sin(x+1)");
}

TEST(Printer, RoundTripStable) {
    // 解析 → 打印 → 再解析求值，结果必须一致
    Env env{{"a", 3.0}, {"b", 4.0}, {"c", 5.0}, {"x", 2.0}};
    const char* cases[] = {
        "a-(b-c)",
        "(a+b)*c",
        "2^3^2",
        "-x^2",
        "(x+1)!",
        "a/(b+c)",
        "2x + 3sin(x)",
        "a-b-c",
        "a^b-c",
        "50%+1",
    };
    for (const char* s : cases) {
        double v1 = evaluate(*parse(s), env);
        double v2 = evaluate(*parse(toString(*parse(s))), env);
        EXPECT_NEAR(v1, v2, 1e-9) << "往返失败: " << s;
    }
}

TEST(Printer, CloneIndependence) {
    NodePtr n = parse("x+1");
    NodePtr c = clone(*n);
    EXPECT_EQ(toString(*c), "x+1");
    c->args[0]->name = "y";
    EXPECT_EQ(toString(*n), "x+1"); // 原节点不受影响
}
