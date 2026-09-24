#pragma once

#include <functional>
#include <vector>

namespace mk {

// 数值方法模块：纯函数集合，参数非法（容差 ≤ 0、步数 < 0 等）或不收敛时
// 抛 MkError（中文消息）；f 返回非有限数（NaN/Inf）同样抛错。

// 牛顿迭代求 f(x)=0 在 x0 附近的根；导数用中心差分近似。
// |Δx| ≤ tol*(1+|x|) 视为收敛；超过 maxIter 步未收敛抛错。
double newtonRoot(const std::function<double(double)>& f, double x0,
                  double tol = 1e-12, int maxIter = 100);

// 自适应辛普森求 f 在 [a,b] 上的定积分，tol 为目标绝对误差。
// 递归深度超过 maxDepth 仍未达标抛错；a == b 时返回 0，a > b 按反向积分处理。
double simpsonIntegral(const std::function<double(double)>& f, double a, double b,
                       double tol = 1e-10, int maxDepth = 20);

struct OdeSample { double x, y; };

// 常微分方程初值问题 y' = f(x,y), y(x0) = y0 的经典四阶 Runge-Kutta 求解。
// 固定步长 h 前进 steps 步，返回 steps+1 个采样点（含初始点）；h < 0 表示反向积分。
std::vector<OdeSample> rungeKutta4(const std::function<double(double, double)>& f,
                                   double x0, double y0, double h, int steps);

} // namespace mk
