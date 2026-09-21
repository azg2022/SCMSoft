#pragma once

#include <cstddef>
#include <vector>

namespace mk {

// 行主序稠密矩阵。边界接口不暴露 Eigen 类型，保持内核 ABI 友好。
// 仅在 MK_HAVE_EIGEN 下提供实现；所有输入矩阵做维度校验，不匹配抛 MkError（中文消息）。
struct Matrix {
    std::size_t rows = 0, cols = 0;
    std::vector<double> data; // 行主序，长度必须为 rows*cols
};

double determinant(const Matrix& m);
Matrix transpose(const Matrix& m);
Matrix multiply(const Matrix& a, const Matrix& b);
Matrix inverse(const Matrix& m); // 奇异矩阵抛"矩阵不可逆（行列式为 0）"

struct EigenResult {
    std::vector<double> eigenvalues;
    std::vector<std::vector<double>> eigenvectors; // 每个特征向量一个 vector<double>
};
EigenResult eigen(const Matrix& m); // 方阵；对称用 SelfAdjointEigenSolver，否则 EigenSolver（复特征值报错）

enum class NormKind { L1, L2, Frobenius, Inf };
double norm(const Matrix& m, NormKind kind);

struct LUResult { Matrix L, U, P; }; // P*A = L*U
LUResult lu(const Matrix& m);

struct QRResult { Matrix Q, R; }; // HouseholderQR，A = Q*R
QRResult qr(const Matrix& m);

struct SVDResult { Matrix U, V; std::vector<double> singularValues; }; // JacobiSVD
SVDResult svd(const Matrix& m);

struct CholeskyResult { Matrix L; }; // 非正定抛错
CholeskyResult cholesky(const Matrix& m);

// 方阵用 partialPivLu，非方阵用 colPivHouseholderQr 最小二乘
Matrix solve(const Matrix& A, const Matrix& b);

} // namespace mk
