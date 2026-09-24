# 度量衡 MeasureKit

融合符号计算、拍照测量与可配置公式引擎的跨平台计算工具。一套 C++17 内核，目标覆盖 Windows / Linux / macOS / Android / HarmonyOS / iOS 六端。

> 当前进度：**v0.1.0** — 内核 + SDK + Linux/Windows 桌面版可用；拍照测量与移动端在路线图中（见下文）。

## 功能

- **符号与工程计算**：四则运算、初等函数、符号微分（含高阶/偏导）、表达式化简、精确分数输出
- **数值方法**：牛顿迭代求根、自适应辛普森积分、经典四阶 Runge-Kutta 解常微分方程初值问题
- **线性代数**：转置、行列式、逆、特征值/特征向量、L1/L2/F/∞ 范数、LU / QR / SVD / Cholesky 分解、线性方程组求解（Eigen）
- **公式引擎**：五大类预置公式库（一般数学 / 概率统计 / 数据分析 / 线性代数 / 微积分），参数可配置，支持自定义公式入库与搜索
- **桌面应用**：Qt 6 Widgets，含计算器键盘、公式库面板、矩阵计算页签

## 目录结构

```
measurekit/
├── kernel/    计算内核（解析 / 求值 / 微分 / 线代 / 公式引擎）+ 单元测试
├── sdk/       C ABI 稳定边界（mk_ 前缀），对外开放内核能力
├── app/       Qt 6 桌面应用
└── formulas/  公式库 schema 与五大类种子公式（JSON）
scripts/release.sh  一键发版脚本
.github/workflows/  Linux / Windows CI（构建、测试、打包、发 Release）
```

## 构建（Linux）

依赖：CMake ≥ 3.22、C++17 编译器、Qt 6（仅桌面应用）、Eigen3、SQLite3、GTest（仅测试）。

```bash
sudo apt install build-essential cmake qt6-base-dev libgl1-mesa-dev \
                 libeigen3-dev libsqlite3-dev libgtest-dev

cmake -S measurekit -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$(nproc)"
```

可用选项：`MK_BUILD_APP`（桌面应用，默认 ON）、`MK_BUILD_TESTS`（测试，默认 ON）、`MK_BUILD_SDK`（SDK，默认 ON）；Eigen / SQLite / GTest 缺失时相应能力自动降级。

## 测试

```bash
ctest --test-dir build --output-on-failure
```

共 80 个单元测试，覆盖词法、语法、求值、化简、微分、线代与公式引擎。

## 打包与发版

```bash
cmake --build build --target package   # Linux 产出 .deb；Windows 产出 NSIS + ZIP
./scripts/release.sh 0.1.1 "说明"      # 更新版本号 → 提交 → 打标签 → 推送，CI 自动发 Release
```

推送到 `main` 触发 CI 构建与测试；打 `v*` 标签额外自动发布到 GitHub Releases。

## 路线图

- **v0.1.x**：工程健康（CI、文档）✅
- **v0.2.0**：数值方法（牛顿求根 / 数值积分 / 常微分方程）✅、分步解析、历史记录与导出
- **v0.3.0**：公式库扩充、macOS 版
- **v0.4.0**：单位换算与输入校验
- **v1.0.0**：Android / HarmonyOS / iOS 端 + SDK 多语言封装
- **v2.x**：拍照测量（端侧轻量模型 + 几何解算）

## 许可证

[MIT](LICENSE)
