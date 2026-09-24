#include "mk/numerics.h"

#include <cmath>

#include "mk/error.h"

namespace mk {
namespace {

void ensureFinite(double v, const char* what) {
    if (!std::isfinite(v))
        throw MkError(std::string(what) + "出现非有限数（NaN 或 Inf），请检查函数在求解区间内的定义");
}

} // namespace

double newtonRoot(const std::function<double(double)>& f, double x0,
                  double tol, int maxIter) {
    if (tol <= 0)
        throw MkError("收敛容差必须为正数");
    if (maxIter < 1)
        throw MkError("最大迭代次数必须 ≥ 1");

    double x = x0;
    for (int i = 0; i < maxIter; ++i) {
        double fx = f(x);
        ensureFinite(fx, "牛顿迭代");

        double h = 1e-6 * (1.0 + std::fabs(x));
        double dfx = (f(x + h) - f(x - h)) / (2.0 * h);
        ensureFinite(dfx, "牛顿迭代");
        if (dfx == 0.0)
            throw MkError("牛顿迭代失败：导数为 0，无法继续（请更换初始值）");

        double step = fx / dfx;
        x -= step;
        if (std::fabs(step) <= tol * (1.0 + std::fabs(x)))
            return x;
    }
    throw MkError("牛顿迭代未收敛：超过最大迭代次数，请更换初始值或放宽容差");
}

namespace {

double simpson(const std::function<double(double)>& f, double a, double b,
               double fa, double fb, double fc) {
    return (b - a) / 6.0 * (fa + 4.0 * fc + fb);
}

double adaptiveSimpson(const std::function<double(double)>& f, double a, double b,
                       double fa, double fb, double fc, double whole,
                       double tol, int depth) {
    double m = (a + b) / 2.0;
    double lm = (a + m) / 2.0;
    double rm = (m + b) / 2.0;
    double flm = f(lm);
    double frm = f(rm);
    ensureFinite(flm, "自适应辛普森积分");
    ensureFinite(frm, "自适应辛普森积分");

    double left = simpson(f, a, m, fa, fc, flm);
    double right = simpson(f, m, b, fc, fb, frm);
    double delta = left + right - whole;

    if (depth <= 0)
        throw MkError("自适应辛普森积分未达到精度要求：递归深度超限，请放宽容差");
    if (std::fabs(delta) <= 15.0 * tol)
        return left + right + delta / 15.0;

    return adaptiveSimpson(f, a, m, fa, fc, flm, left, tol / 2.0, depth - 1)
         + adaptiveSimpson(f, m, b, fc, fb, frm, right, tol / 2.0, depth - 1);
}

} // namespace

double simpsonIntegral(const std::function<double(double)>& f, double a, double b,
                       double tol, int maxDepth) {
    if (tol <= 0)
        throw MkError("积分容差必须为正数");
    if (maxDepth < 1)
        throw MkError("最大递归深度必须 ≥ 1");
    if (a == b)
        return 0.0;

    double m = (a + b) / 2.0;
    double fa = f(a), fb = f(b), fc = f(m);
    ensureFinite(fa, "自适应辛普森积分");
    ensureFinite(fb, "自适应辛普森积分");
    ensureFinite(fc, "自适应辛普森积分");

    double whole = simpson(f, a, b, fa, fb, fc);
    return adaptiveSimpson(f, a, b, fa, fb, fc, whole, tol, maxDepth);
}

std::vector<OdeSample> rungeKutta4(const std::function<double(double, double)>& f,
                                   double x0, double y0, double h, int steps) {
    if (steps < 0)
        throw MkError("积分步数必须 ≥ 0");
    if (h == 0.0)
        throw MkError("步长 h 不能为 0");

    std::vector<OdeSample> out;
    out.reserve(static_cast<std::size_t>(steps) + 1);
    double x = x0, y = y0;
    out.push_back({x, y});

    for (int i = 0; i < steps; ++i) {
        double k1 = f(x, y);
        ensureFinite(k1, "Runge-Kutta 求解");
        double k2 = f(x + h / 2.0, y + h * k1 / 2.0);
        ensureFinite(k2, "Runge-Kutta 求解");
        double k3 = f(x + h / 2.0, y + h * k2 / 2.0);
        ensureFinite(k3, "Runge-Kutta 求解");
        double k4 = f(x + h, y + h * k3);
        ensureFinite(k4, "Runge-Kutta 求解");

        y += h / 6.0 * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
        x += h;
        out.push_back({x, y});
    }
    return out;
}

} // namespace mk
