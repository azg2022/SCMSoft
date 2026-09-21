#include <gtest/gtest.h>

#include "mk/error.h"
#include "mk/tokenizer.h"

using namespace mk;

TEST(Tokenizer, Numbers) {
    auto toks = tokenize("42 2.5 .5");
    ASSERT_EQ(toks.size(), 4); // 3 个数字 + End
    EXPECT_EQ(toks[0].kind, TokKind::Number);
    EXPECT_DOUBLE_EQ(toks[0].num, 42.0);
    EXPECT_DOUBLE_EQ(toks[1].num, 2.5);
    EXPECT_DOUBLE_EQ(toks[2].num, 0.5);
    EXPECT_EQ(toks[3].kind, TokKind::End);
}

TEST(Tokenizer, ScientificNotation) {
    auto toks = tokenize("2e3 1.5E-2 3e+2");
    ASSERT_EQ(toks.size(), 4);
    EXPECT_DOUBLE_EQ(toks[0].num, 2000.0);
    EXPECT_DOUBLE_EQ(toks[1].num, 0.015);
    EXPECT_DOUBLE_EQ(toks[2].num, 300.0);
}

TEST(Tokenizer, EWithoutDigitsIsIdent) {
    // 2e 中的 e 不是指数（后面没数字），应为 Number(2) + Ident(e)
    auto toks = tokenize("2e");
    ASSERT_EQ(toks.size(), 3);
    EXPECT_EQ(toks[0].kind, TokKind::Number);
    EXPECT_DOUBLE_EQ(toks[0].num, 2.0);
    EXPECT_EQ(toks[1].kind, TokKind::Ident);
    EXPECT_EQ(toks[1].text, "e");
}

TEST(Tokenizer, Idents) {
    auto toks = tokenize("sin x alpha_2");
    ASSERT_EQ(toks.size(), 4);
    EXPECT_EQ(toks[0].kind, TokKind::Ident);
    EXPECT_EQ(toks[0].text, "sin");
    EXPECT_EQ(toks[1].text, "x");
    EXPECT_EQ(toks[2].text, "alpha_2");
}

TEST(Tokenizer, OpsAndParens) {
    auto toks = tokenize("+-*/^!%(),");
    ASSERT_EQ(toks.size(), 11);
    for (int i = 0; i < 7; ++i)
        EXPECT_EQ(toks[i].kind, TokKind::Op);
    EXPECT_EQ(toks[7].kind, TokKind::LParen);
    EXPECT_EQ(toks[8].kind, TokKind::RParen);
    EXPECT_EQ(toks[9].kind, TokKind::Comma);
}

TEST(Tokenizer, SkipsWhitespace) {
    auto toks = tokenize("  1  +\t2 \n");
    ASSERT_EQ(toks.size(), 4);
    EXPECT_DOUBLE_EQ(toks[0].num, 1.0);
    EXPECT_EQ(toks[1].kind, TokKind::Op);
    EXPECT_DOUBLE_EQ(toks[2].num, 2.0);
}

TEST(Tokenizer, IllegalCharPosition) {
    try {
        tokenize("1 + @");
        FAIL() << "应抛出 MkError";
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "不支持的字符 '@'");
        EXPECT_EQ(e.pos(), 4u);
    }
}

TEST(Tokenizer, TokenPositions) {
    auto toks = tokenize("x + 12");
    EXPECT_EQ(toks[0].pos, 0u);
    EXPECT_EQ(toks[1].pos, 2u);
    EXPECT_EQ(toks[2].pos, 4u);
}
