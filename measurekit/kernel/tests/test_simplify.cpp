#include <gtest/gtest.h>

#include "mk/parser.h"
#include "mk/printer.h"
#include "mk/simplify.h"

using namespace mk;

static std::string simp(const std::string& s) {
    return toString(*simplify(parse(s)));
}

TEST(Simplify, ConstantFolding) {
    EXPECT_EQ(simp("2+3*4"), "14");
    EXPECT_EQ(simp("2^10"), "1024");
    EXPECT_EQ(simp("sin(0)"), "0");
    EXPECT_EQ(simp("0/0"), "0/0"); // 求值抛异常则保留原样
}

TEST(Simplify, AdditiveIdentity) {
    EXPECT_EQ(simp("x+0"), "x");
    EXPECT_EQ(simp("0+x"), "x");
    EXPECT_EQ(simp("x-0"), "x");
    EXPECT_EQ(simp("0-x"), "-x");
}

TEST(Simplify, MultiplicativeRules) {
    EXPECT_EQ(simp("x*1"), "x");
    EXPECT_EQ(simp("1*x"), "x");
    EXPECT_EQ(simp("x*0"), "0");
    EXPECT_EQ(simp("0*x"), "0");
}

TEST(Simplify, DivisionRules) {
    EXPECT_EQ(simp("0/x"), "0");
    EXPECT_EQ(simp("x/1"), "x");
}

TEST(Simplify, PowerRules) {
    EXPECT_EQ(simp("x^1"), "x");
    EXPECT_EQ(simp("x^0"), "1");
    EXPECT_EQ(simp("1^x"), "1");
    EXPECT_EQ(simp("0^x"), "0");
}

TEST(Simplify, DoubleNegation) {
    EXPECT_EQ(simp("-(-x)"), "x");
}

TEST(Simplify, LikeTerms) {
    EXPECT_EQ(simp("2*x+3*x"), "5*x");
    EXPECT_EQ(simp("x+x"), "2*x");
}

TEST(Simplify, CoefMerge) {
    EXPECT_EQ(simp("4*(3*x)"), "12*x"); // 常数系数结合
}

TEST(Simplify, PercentConversion) {
    EXPECT_EQ(simp("x%"), "x/100");
}

TEST(Simplify, Idempotent) {
    // 化简结果再化简应不变（不动点）
    NodePtr once = simplify(parse("x*1+0*(y+1)"));
    NodePtr twice = simplify(clone(*once));
    EXPECT_EQ(toString(*once), toString(*twice));
    EXPECT_EQ(toString(*once), "x");
}
