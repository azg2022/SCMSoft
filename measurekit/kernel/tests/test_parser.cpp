#include <gtest/gtest.h>

#include "mk/error.h"
#include "mk/evaluator.h"
#include "mk/parser.h"

using namespace mk;

static double eval(const std::string& src, const Env& env = {}) {
    return evaluate(*parse(src), env);
}

TEST(Parser, Precedence) {
    EXPECT_DOUBLE_EQ(eval("2+3*4"), 14.0);
    EXPECT_DOUBLE_EQ(eval("(2+3)*4"), 20.0);
}

TEST(Parser, PowerRightAssociative) {
    EXPECT_DOUBLE_EQ(eval("2^3^2"), 512.0); // 2^(3^2) 而非 (2^3)^2
}

TEST(Parser, UnaryMinusLowerThanPower) {
    EXPECT_DOUBLE_EQ(eval("-2^2"), -4.0); // -(2^2)
    EXPECT_DOUBLE_EQ(eval("2^-3"), 0.125);
}

TEST(Parser, ImplicitMultiplication) {
    EXPECT_DOUBLE_EQ(eval("2x", {{"x", 3.0}}), 6.0);
    EXPECT_DOUBLE_EQ(eval("2(3+4)"), 14.0);
    EXPECT_DOUBLE_EQ(eval("2sin(0)"), 0.0);
    EXPECT_DOUBLE_EQ(eval("x sin(pi/2)", {{"x", 5.0}}), 5.0);
}

TEST(Parser, FunctionCall) {
    NodePtr n = parse("sin(x)");
    ASSERT_EQ(n->kind, Node::Kind::Call);
    EXPECT_EQ(n->name, "sin");
    ASSERT_EQ(n->args.size(), 1u);
    EXPECT_EQ(n->args[0]->kind, Node::Kind::Variable);
}

TEST(Parser, MultiArgCall) {
    NodePtr n = parse("log(2,8)");
    ASSERT_EQ(n->kind, Node::Kind::Call);
    EXPECT_EQ(n->args.size(), 2u);
}

TEST(Parser, IdentWithoutParenIsVariable) {
    NodePtr n = parse("sin");
    EXPECT_EQ(n->kind, Node::Kind::Variable);
    EXPECT_EQ(n->name, "sin");
}

TEST(Parser, PostfixOps) {
    NodePtr n = parse("5!");
    ASSERT_EQ(n->kind, Node::Kind::Unary);
    EXPECT_EQ(n->op, '!');
    n = parse("50%");
    ASSERT_EQ(n->kind, Node::Kind::Unary);
    EXPECT_EQ(n->op, '%');
}

TEST(Parser, TrailingGarbage) {
    try {
        parse("1+2 3)");
        FAIL() << "应抛出 MkError";
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "表达式末尾有多余内容");
    }
}

TEST(Parser, ErrorPositions) {
    try {
        parse("1 + * 2");
        FAIL() << "应抛出 MkError";
    } catch (const MkError& e) {
        EXPECT_EQ(e.pos(), 4u); // '*' 所在位置
    }
    try {
        parse("(1+2");
        FAIL() << "应抛出 MkError";
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "缺少右括号 ')'");
    }
}

TEST(Parser, EmptyExpression) {
    EXPECT_THROW(parse(""), MkError);
    EXPECT_THROW(parse("   "), MkError);
}
