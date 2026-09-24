#include <vector>

#include <gtest/gtest.h>

#include "mk/error.h"
#include "mk/evaluator.h"
#include "mk/parser.h"

namespace {

// 便捷函数：解析 expr 并做分步求值
std::vector<mk::EvalStep> stepsOf(const char* expr, const mk::Env& env = {}) {
    mk::NodePtr ast = mk::parse(expr);
    std::vector<mk::EvalStep> steps;
    mk::evaluate(*ast, env, &steps);
    return steps;
}

} // namespace

TEST(EvalSteps, SimpleArithmetic) {
    // 2+3*4：后序，先算 3*4 再算整体
    auto steps = stepsOf("2+3*4");
    ASSERT_EQ(steps.size(), 2u);
    EXPECT_EQ(steps[0].expr, "3*4");
    EXPECT_DOUBLE_EQ(steps[0].value, 12.0);
    EXPECT_EQ(steps[1].expr, "2+3*4");
    EXPECT_DOUBLE_EQ(steps[1].value, 14.0);
}

TEST(EvalSteps, VariableSubstitution) {
    // x^2+1（x=2）：先代入 x，再算 x^2，最后整体
    auto steps = stepsOf("x^2+1", {{"x", 2.0}});
    ASSERT_EQ(steps.size(), 3u);
    EXPECT_EQ(steps[0].expr, "x");
    EXPECT_DOUBLE_EQ(steps[0].value, 2.0);
    EXPECT_EQ(steps[1].expr, "x^2");
    EXPECT_DOUBLE_EQ(steps[1].value, 4.0);
    EXPECT_EQ(steps[2].expr, "x^2+1");
    EXPECT_DOUBLE_EQ(steps[2].value, 5.0);
}

TEST(EvalSteps, BuiltinConstant) {
    auto steps = stepsOf("2*pi");
    ASSERT_EQ(steps.size(), 2u);
    EXPECT_EQ(steps[0].expr, "pi");
    EXPECT_NEAR(steps[0].value, 3.141592653589793, 1e-15);
    EXPECT_EQ(steps[1].expr, "2*pi");
    EXPECT_NEAR(steps[1].value, 6.283185307179586, 1e-12);
}

TEST(EvalSteps, FunctionCall) {
    auto steps = stepsOf("sqrt(9)+1");
    ASSERT_EQ(steps.size(), 2u);
    EXPECT_EQ(steps[0].expr, "sqrt(9)");
    EXPECT_DOUBLE_EQ(steps[0].value, 3.0);
    EXPECT_EQ(steps[1].expr, "sqrt(9)+1");
    EXPECT_DOUBLE_EQ(steps[1].value, 4.0);
}

TEST(EvalSteps, TwoArgumentLog) {
    auto steps = stepsOf("log(2,8)");
    ASSERT_EQ(steps.size(), 1u);
    EXPECT_EQ(steps[0].expr, "log(2,8)");
    EXPECT_DOUBLE_EQ(steps[0].value, 3.0);
}

TEST(EvalSteps, NullptrMatchesPlainEvaluate) {
    mk::NodePtr ast = mk::parse("(1+2)*(3+4)");
    mk::Env env;
    EXPECT_DOUBLE_EQ(mk::evaluate(*ast, env), mk::evaluate(*ast, env, nullptr));
}

TEST(EvalSteps, RepeatedVariableRecordedEachOccurrence) {
    // x 出现两次，每次代入各产生一条步骤
    auto steps = stepsOf("x+x", {{"x", 3.0}});
    ASSERT_EQ(steps.size(), 3u);
    EXPECT_EQ(steps[0].expr, "x");
    EXPECT_EQ(steps[1].expr, "x");
    EXPECT_DOUBLE_EQ(steps[2].value, 6.0);
}

TEST(EvalSteps, ErrorKeepsCompletedSteps) {
    // 1/0 在除法节点抛错：除法及外层步骤未记录，已完成步骤保留（此处均为数字字面量，故为空）
    mk::NodePtr ast = mk::parse("(1+2)/0");
    std::vector<mk::EvalStep> steps;
    EXPECT_THROW(mk::evaluate(*ast, {}, &steps), mk::MkError);
    // 先完成 1+2 的步骤，再在除法处失败
    ASSERT_EQ(steps.size(), 1u);
    EXPECT_EQ(steps[0].expr, "1+2");
    EXPECT_DOUBLE_EQ(steps[0].value, 3.0);
}

TEST(EvalSteps, FinalStepIsWholeExpression) {
    auto steps = stepsOf("(1+2)*(3-4)/5");
    ASSERT_FALSE(steps.empty());
    EXPECT_EQ(steps.back().expr, "(1+2)*(3-4)/5");
    EXPECT_DOUBLE_EQ(steps.back().value, -0.6);
}
