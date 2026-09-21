#include <gtest/gtest.h>

// 线性代数测试仅在 Eigen 可用（MK_HAVE_EIGEN）时有意义；
// tests/CMakeLists.txt 也按 Eigen3_FOUND 条件编译本文件，双重保险。
#ifdef MK_HAVE_EIGEN

#include <algorithm>
#include <cmath>
#include <initializer_list>

#include "mk/error.h"
#include "mk/linalg.h"

using namespace mk;

namespace {

Matrix mat(std::size_t r, std::size_t c, std::initializer_list<double> d) {
    return {r, c, std::vector<double>(d)};
}

Matrix ident(std::size_t n) {
    Matrix m{n, n, std::vector<double>(n * n, 0.0)};
    for (std::size_t i = 0; i < n; ++i)
        m.data[i * n + i] = 1.0;
    return m;
}

double maxAbsDiff(const Matrix& a, const Matrix& b) {
    if (a.rows != b.rows || a.cols != b.cols)
        return 1e300;
    double m = 0.0;
    for (std::size_t i = 0; i < a.data.size(); ++i)
        m = std::max(m, std::abs(a.data[i] - b.data[i]));
    return m;
}

// 经典 3x3 测试矩阵：det = 1，逆矩阵为整数矩阵
Matrix A() { return mat(3, 3, {1, 2, 3, 0, 1, 4, 5, 6, 0}); }

} // namespace

TEST(Linalg, Determinant) {
    EXPECT_NEAR(determinant(A()), 1.0, 1e-10);
    EXPECT_NEAR(determinant(mat(2, 2, {1, 2, 3, 4})), -2.0, 1e-10);
    EXPECT_THROW(determinant(mat(2, 3, {1, 2, 3, 4, 5, 6})), MkError); // 非方阵
}

TEST(Linalg, TransposeMultiply) {
    Matrix t = transpose(A());
    EXPECT_EQ(t.rows, 3u);
    EXPECT_NEAR(t.data[1], 0.0, 1e-15); // A[0][1]=2 → T[1][0]=2, T[0][1]=A[1][0]=0
    EXPECT_NEAR(t.data[3], 2.0, 1e-15);

    Matrix prod = multiply(A(), ident(3));
    EXPECT_NEAR(maxAbsDiff(prod, A()), 0.0, 1e-12);
    EXPECT_THROW(multiply(A(), mat(2, 2, {1, 2, 3, 4})), MkError); // 维度不匹配
}

TEST(Linalg, Inverse) {
    // 手算逆矩阵（det=1，伴随矩阵即逆）
    Matrix expected = mat(3, 3, {-24, 18, 5, 20, -15, -4, -5, 4, 1});
    Matrix inv = inverse(A());
    EXPECT_NEAR(maxAbsDiff(inv, expected), 0.0, 1e-10);
    EXPECT_NEAR(maxAbsDiff(multiply(A(), inv), ident(3)), 0.0, 1e-10);

    try {
        inverse(mat(2, 2, {1, 2, 2, 4}));
        FAIL();
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "矩阵不可逆（行列式为 0）");
    }
}

TEST(Linalg, EigenSymmetric) {
    // 对称矩阵，特征值手算为 {3, 3, 6}
    Matrix b = mat(3, 3, {4, 1, 1, 1, 4, 1, 1, 1, 4});
    EigenResult r = eigen(b);
    ASSERT_EQ(r.eigenvalues.size(), 3u);
    std::vector<double> sorted = r.eigenvalues;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_NEAR(sorted[0], 3.0, 1e-10);
    EXPECT_NEAR(sorted[1], 3.0, 1e-10);
    EXPECT_NEAR(sorted[2], 6.0, 1e-10);

    // 验证每对 (λ, v) 满足 B v = λ v
    for (std::size_t k = 0; k < 3; ++k) {
        Matrix v{3, 1, r.eigenvectors[k]};
        Matrix bv = multiply(b, v);
        for (std::size_t i = 0; i < 3; ++i)
            EXPECT_NEAR(bv.data[i], r.eigenvalues[k] * v.data[i], 1e-10);
    }
}

TEST(Linalg, EigenNonsymmetric) {
    // 特征值 = (5±√33)/2
    EigenResult r = eigen(mat(2, 2, {1, 2, 3, 4}));
    ASSERT_EQ(r.eigenvalues.size(), 2u);
    std::vector<double> sorted = r.eigenvalues;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_NEAR(sorted[0], (5.0 - std::sqrt(33.0)) / 2.0, 1e-10);
    EXPECT_NEAR(sorted[1], (5.0 + std::sqrt(33.0)) / 2.0, 1e-10);
}

TEST(Linalg, EigenComplexThrows) {
    // 旋转矩阵特征值为 ±i
    try {
        eigen(mat(2, 2, {0, -1, 1, 0}));
        FAIL();
    } catch (const MkError& e) {
        EXPECT_EQ(std::string(e.what()), "该矩阵含复特征值，暂不支持");
    }
}

TEST(Linalg, Norms) {
    Matrix c = mat(2, 2, {1, 2, 3, 4});
    EXPECT_NEAR(norm(c, NormKind::L1), 6.0, 1e-10);        // 最大列绝对和
    EXPECT_NEAR(norm(c, NormKind::Inf), 7.0, 1e-10);       // 最大行绝对和
    EXPECT_NEAR(norm(c, NormKind::Frobenius), std::sqrt(30.0), 1e-10);
    // 谱范数：AᵀA = [[10,14],[14,20]]，σ_max = sqrt((30+√884)/2)
    EXPECT_NEAR(norm(c, NormKind::L2),
                std::sqrt((30.0 + std::sqrt(884.0)) / 2.0), 1e-10);
}

TEST(Linalg, LUReconstruct) {
    // P*A = L*U
    LUResult r = lu(A());
    EXPECT_NEAR(maxAbsDiff(multiply(r.P, A()), multiply(r.L, r.U)), 0.0, 1e-10);
}

TEST(Linalg, QRReconstruct) {
    QRResult r = qr(A());
    EXPECT_NEAR(maxAbsDiff(multiply(r.Q, r.R), A()), 0.0, 1e-10);
    // Q 正交：QᵀQ = I
    EXPECT_NEAR(maxAbsDiff(multiply(transpose(r.Q), r.Q), ident(3)), 0.0, 1e-10);
}

TEST(Linalg, SVDReconstruct) {
    SVDResult r = svd(A());
    // U Σ Vᵀ = A
    Matrix sigma = mat(3, 3, {});
    sigma.data.assign(9, 0.0);
    for (std::size_t i = 0; i < r.singularValues.size(); ++i)
        sigma.data[i * 3 + i] = r.singularValues[i];
    Matrix recon = multiply(multiply(r.U, sigma), transpose(r.V));
    EXPECT_NEAR(maxAbsDiff(recon, A()), 0.0, 1e-10);
    // 奇异值 = sqrt(AᵀA 的特征值)
    ASSERT_EQ(r.singularValues.size(), 3u);
    for (double s : r.singularValues)
        EXPECT_GE(s, 0.0);
}

TEST(Linalg, Cholesky) {
    Matrix spd = mat(2, 2, {4, 2, 2, 3});
    CholeskyResult r = cholesky(spd);
    EXPECT_NEAR(maxAbsDiff(multiply(r.L, transpose(r.L)), spd), 0.0, 1e-10);
    EXPECT_THROW(cholesky(mat(2, 2, {1, 2, 2, 1})), MkError); // 非正定
}

TEST(Linalg, Solve) {
    // 3 元方程组，已知解 x = (1, -2, -2)
    Matrix a = mat(3, 3, {3, 2, -1, 2, -2, 4, -1, 0.5, -1});
    Matrix b = mat(3, 1, {1, -2, 0});
    Matrix x = solve(a, b);
    ASSERT_EQ(x.rows, 3u);
    EXPECT_NEAR(x.data[0], 1.0, 1e-10);
    EXPECT_NEAR(x.data[1], -2.0, 1e-10);
    EXPECT_NEAR(x.data[2], -2.0, 1e-10);
    // 回代验证
    EXPECT_NEAR(maxAbsDiff(multiply(a, x), b), 0.0, 1e-10);

    // 非方阵最小二乘：超定方程组
    Matrix a2 = mat(3, 2, {1, 1, 1, 2, 1, 3});
    Matrix b2 = mat(3, 1, {1, 2, 2});
    Matrix x2 = solve(a2, b2);
    ASSERT_EQ(x2.rows, 2u);
    // 残差应与 A 的列正交（最小二乘正规方程 Aᵀ(Ax-b)=0）
    Matrix r = mat(3, 1, {});
    Matrix axb = multiply(a2, x2);
    for (std::size_t i = 0; i < 3; ++i)
        axb.data[i] -= b2.data[i];
    Matrix lhs = multiply(transpose(a2), axb);
    EXPECT_NEAR(lhs.data[0], 0.0, 1e-9);
    EXPECT_NEAR(lhs.data[1], 0.0, 1e-9);

    EXPECT_THROW(solve(a, mat(2, 1, {1, 2})), MkError); // 行数不匹配
}

TEST(Linalg, InvalidData) {
    Matrix bad{2, 2, {1, 2, 3}}; // data 长度 != rows*cols
    EXPECT_THROW(determinant(bad), MkError);
}

#endif // MK_HAVE_EIGEN
