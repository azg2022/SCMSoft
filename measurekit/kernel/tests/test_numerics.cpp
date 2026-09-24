#include <cmath>
#include <functional>
#include <limits>

#include <gtest/gtest.h>

#include "mk/error.h"
#include "mk/numerics.h"

using mk::OdeSample;

// ---------- 牛顿求根 ----------

TEST(NumericsNewton, Sqrt2) {
    auto f = [](double x) { return x * x - 2.0; };
    EXPECT_NEAR(mk::newtonRoot(f, 1.0), std::sqrt(2.0), 1e-10);
}

TEST(NumericsNewton, CosFixedPoint) {
    auto f = [](double x) { return std::cos(x) - x; };
    EXPECT_NEAR(mk::newtonRoot(f, 0.5), 0.7390851332151607, 1e-10);
}

TEST(NumericsNewton, MultipleRootConverges) {
    // 重根处收敛为线性（每步误差 × 2/3），且数值差分在 |d| < h 后有 ~h² 的震荡下限，
    // 根精度随容差放宽而趋近该下限（tol=1e-10 时约 1e-7 量级）
    auto f = [](double x) { double d = x - 1.0; return d * d * d; };
    EXPECT_NEAR(mk::newtonRoot(f, 2.0, 1e-10, 300), 1.0, 1e-6);
}

TEST(NumericsNewton, NegativeInitialValue) {
    auto f = [](double x) { return x * x * x + 1.0; };
    EXPECT_NEAR(mk::newtonRoot(f, -2.0), -1.0, 1e-10);
}

TEST(NumericsNewton, NoRealRootThrows) {
    auto f = [](double x) { return x * x + 1.0; };
    EXPECT_THROW(mk::newtonRoot(f, 0.5), mk::MkError);
}

TEST(NumericsNewton, FlatDerivativeThrows) {
    auto f = [](double) { return 3.0; };
    EXPECT_THROW(mk::newtonRoot(f, 1.0), mk::MkError);
}

TEST(NumericsNewton, NonFiniteThrows) {
    auto f = [](double) { return std::numeric_limits<double>::infinity(); };
    EXPECT_THROW(mk::newtonRoot(f, 1.0), mk::MkError);
}

TEST(NumericsNewton, InvalidToleranceThrows) {
    auto f = [](double x) { return x - 1.0; };
    EXPECT_THROW(mk::newtonRoot(f, 2.0, 0.0), mk::MkError);
    EXPECT_THROW(mk::newtonRoot(f, 2.0, -1e-9), mk::MkError);
}

// ---------- 自适应辛普森积分 ----------

TEST(NumericsSimpson, QuadraticExact) {
    auto f = [](double x) { return x * x; };
    EXPECT_NEAR(mk::simpsonIntegral(f, 0.0, 1.0), 1.0 / 3.0, 1e-12);
}

TEST(NumericsSimpson, SineOverHalfPeriod) {
    auto f = [](double x) { return std::sin(x); };
    EXPECT_NEAR(mk::simpsonIntegral(f, 0.0, M_PI), 2.0, 1e-10);
}

TEST(NumericsSimpson, ReversedLimitsNegate) {
    auto f = [](double x) { return x * x; };
    EXPECT_NEAR(mk::simpsonIntegral(f, 1.0, 0.0), -1.0 / 3.0, 1e-12);
}

TEST(NumericsSimpson, EqualLimitsZero) {
    auto f = [](double x) { return std::sin(x) + x; };
    EXPECT_DOUBLE_EQ(mk::simpsonIntegral(f, 0.7, 0.7), 0.0);
}

TEST(NumericsSimpson, TighterToleranceMoreAccurate) {
    auto f = [](double x) { return std::exp(x) * std::sin(x); };
    // 参考值由高精度数值库给出
    const double ref = (M_E * (std::sin(1.0) - std::cos(1.0)) + 1.0) / 2.0;
    double loose = mk::simpsonIntegral(f, 0.0, 1.0, 1e-6);
    double tight = mk::simpsonIntegral(f, 0.0, 1.0, 1e-13);
    EXPECT_GT(std::fabs(loose - ref), std::fabs(tight - ref));
    EXPECT_NEAR(tight, ref, 1e-12);
}

TEST(NumericsSimpson, NonFiniteThrows) {
    auto f = [](double x) { return 1.0 / (x - 0.5); };
    EXPECT_THROW(mk::simpsonIntegral(f, 0.0, 1.0), mk::MkError);
}

TEST(NumericsSimpson, DepthLimitThrows) {
    // sqrt 在 0 处导数奇异，精度要求极高时递归必然超限
    auto f = [](double x) { return std::sqrt(x); };
    EXPECT_THROW(mk::simpsonIntegral(f, 0.0, 1.0, 1e-15, 4), mk::MkError);
}

TEST(NumericsSimpson, InvalidArgsThrow) {
    auto f = [](double x) { return x; };
    EXPECT_THROW(mk::simpsonIntegral(f, 0.0, 1.0, 0.0), mk::MkError);
    EXPECT_THROW(mk::simpsonIntegral(f, 0.0, 1.0, 1e-10, 0), mk::MkError);
}

// ---------- Runge-Kutta 常微分方程 ----------

TEST(NumericsRk4, ExponentialGrowth) {
    auto f = [](double, double y) { return y; };
    auto out = mk::rungeKutta4(f, 0.0, 1.0, 0.05, 20);
    ASSERT_EQ(out.size(), 21u);
    EXPECT_NEAR(out.back().x, 1.0, 1e-15);
    EXPECT_NEAR(out.back().y, M_E, 1e-6);
}

TEST(NumericsRk4, ExponentialDecay) {
    auto f = [](double, double y) { return -2.0 * y; };
    auto out = mk::rungeKutta4(f, 0.0, 1.0, 0.05, 20);
    EXPECT_NEAR(out.back().y, std::exp(-2.0), 1e-6);
}

TEST(NumericsRk4, ZeroStepsReturnsInitialPoint) {
    auto f = [](double, double y) { return y; };
    auto out = mk::rungeKutta4(f, 2.0, 3.0, 0.1, 0);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_DOUBLE_EQ(out[0].x, 2.0);
    EXPECT_DOUBLE_EQ(out[0].y, 3.0);
}

TEST(NumericsRk4, BackwardIntegration) {
    auto f = [](double, double y) { return y; };
    auto out = mk::rungeKutta4(f, 1.0, M_E, -0.05, 20);
    EXPECT_NEAR(out.back().x, 0.0, 1e-15);
    EXPECT_NEAR(out.back().y, 1.0, 1e-6);
}

TEST(NumericsRk4, DependentVariableTerm) {
    // y' = x + y, y(0)=1 的解析解为 y = 2e^x - x - 1，y(1) = 2e - 2
    auto f = [](double x, double y) { return x + y; };
    auto out = mk::rungeKutta4(f, 0.0, 1.0, 0.05, 20);
    EXPECT_NEAR(out.back().y, 2.0 * M_E - 2.0, 1e-6);
}

TEST(NumericsRk4, SingularityThrows) {
    // 第一步的半步采样点恰好命中奇点 x=0.5
    auto f = [](double x, double) { return 1.0 / (x - 0.5); };
    EXPECT_THROW(mk::rungeKutta4(f, 0.0, 0.0, 1.0, 1), mk::MkError);
}

TEST(NumericsRk4, InvalidArgsThrow) {
    auto f = [](double, double y) { return y; };
    EXPECT_THROW(mk::rungeKutta4(f, 0.0, 1.0, 0.1, -1), mk::MkError);
    EXPECT_THROW(mk::rungeKutta4(f, 0.0, 1.0, 0.0, 1), mk::MkError);
}
