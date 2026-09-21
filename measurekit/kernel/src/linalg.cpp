#include "mk/linalg.h"

#ifdef MK_HAVE_EIGEN

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include <cmath>
#include <string>

#include "mk/error.h"

namespace mk {

namespace {

using EMat = Eigen::MatrixXd;

void checkValid(const Matrix& m) {
    if (m.rows == 0 || m.cols == 0 || m.data.size() != m.rows * m.cols)
        throw MkError("矩阵数据长度与维度不匹配");
}

void checkSquare(const Matrix& m, const char* what) {
    checkValid(m);
    if (m.rows != m.cols)
        throw MkError(std::string(what) + "要求方阵");
}

EMat toEigen(const Matrix& m) {
    EMat e(static_cast<Eigen::Index>(m.rows), static_cast<Eigen::Index>(m.cols));
    for (std::size_t i = 0; i < m.rows; ++i)
        for (std::size_t j = 0; j < m.cols; ++j)
            e(static_cast<Eigen::Index>(i), static_cast<Eigen::Index>(j)) =
                m.data[i * m.cols + j];
    return e;
}

Matrix fromEigen(const EMat& e) {
    Matrix m;
    m.rows = static_cast<std::size_t>(e.rows());
    m.cols = static_cast<std::size_t>(e.cols());
    m.data.resize(m.rows * m.cols);
    for (std::size_t i = 0; i < m.rows; ++i)
        for (std::size_t j = 0; j < m.cols; ++j)
            m.data[i * m.cols + j] = e(static_cast<Eigen::Index>(i), static_cast<Eigen::Index>(j));
    return m;
}

} // namespace

double determinant(const Matrix& m) {
    checkSquare(m, "行列式");
    return toEigen(m).determinant();
}

Matrix transpose(const Matrix& m) {
    checkValid(m);
    return fromEigen(toEigen(m).transpose());
}

Matrix multiply(const Matrix& a, const Matrix& b) {
    checkValid(a);
    checkValid(b);
    if (a.cols != b.rows)
        throw MkError("矩阵乘法维度不匹配");
    return fromEigen(toEigen(a) * toEigen(b));
}

Matrix inverse(const Matrix& m) {
    checkSquare(m, "求逆");
    EMat a = toEigen(m);
    if (std::abs(a.determinant()) < 1e-12)
        throw MkError("矩阵不可逆（行列式为 0）");
    return fromEigen(a.inverse());
}

EigenResult eigen(const Matrix& m) {
    checkSquare(m, "特征值分解");
    EMat a = toEigen(m);
    EigenResult res;
    const std::size_t n = m.rows;
    res.eigenvectors.resize(n);

    if (a.isApprox(a.transpose(), 1e-12)) {
        // 对称矩阵：SelfAdjointEigenSolver，特征值必为实数且按升序返回
        Eigen::SelfAdjointEigenSolver<EMat> solver(a);
        if (solver.info() != Eigen::Success)
            throw MkError("特征值分解失败");
        for (std::size_t i = 0; i < n; ++i) {
            res.eigenvalues.push_back(solver.eigenvalues()(static_cast<Eigen::Index>(i)));
            auto v = solver.eigenvectors().col(static_cast<Eigen::Index>(i));
            for (std::size_t j = 0; j < n; ++j)
                res.eigenvectors[i].push_back(v(static_cast<Eigen::Index>(j)));
        }
    } else {
        // 非对称矩阵：EigenSolver，含复特征值时暂不支持
        Eigen::EigenSolver<EMat> solver(a);
        if (solver.info() != Eigen::Success)
            throw MkError("特征值分解失败");
        auto values = solver.eigenvalues();
        auto vectors = solver.eigenvectors();
        for (std::size_t i = 0; i < n; ++i) {
            if (std::abs(values(static_cast<Eigen::Index>(i)).imag()) > 1e-10)
                throw MkError("该矩阵含复特征值，暂不支持");
            res.eigenvalues.push_back(values(static_cast<Eigen::Index>(i)).real());
            for (std::size_t j = 0; j < n; ++j)
                res.eigenvectors[i].push_back(
                    vectors(static_cast<Eigen::Index>(j), static_cast<Eigen::Index>(i)).real());
        }
    }
    return res;
}

double norm(const Matrix& m, NormKind kind) {
    checkValid(m);
    EMat a = toEigen(m);
    switch (kind) {
    case NormKind::L1:
        return a.cwiseAbs().colwise().sum().maxCoeff();
    case NormKind::Inf:
        return a.cwiseAbs().rowwise().sum().maxCoeff();
    case NormKind::Frobenius:
        return a.norm();
    case NormKind::L2: // 谱范数 = 最大奇异值
        return Eigen::JacobiSVD<EMat>(a).singularValues()(0);
    }
    return 0.0;
}

LUResult lu(const Matrix& m) {
    checkSquare(m, "LU 分解");
    Eigen::PartialPivLU<EMat> solver(toEigen(m));
    EMat L = solver.matrixLU().triangularView<Eigen::UnitLower>();
    EMat U = solver.matrixLU().triangularView<Eigen::Upper>();
    EMat P = solver.permutationP().toDenseMatrix().template cast<double>();
    return {fromEigen(L), fromEigen(U), fromEigen(P)};
}

QRResult qr(const Matrix& m) {
    checkValid(m);
    Eigen::HouseholderQR<EMat> solver(toEigen(m));
    EMat Q = solver.householderQ();
    EMat R = solver.matrixQR().triangularView<Eigen::Upper>();
    return {fromEigen(Q), fromEigen(R)};
}

SVDResult svd(const Matrix& m) {
    checkValid(m);
    Eigen::JacobiSVD<EMat> solver(toEigen(m), Eigen::ComputeFullU | Eigen::ComputeFullV);
    SVDResult res;
    res.U = fromEigen(solver.matrixU());
    res.V = fromEigen(solver.matrixV());
    for (Eigen::Index i = 0; i < solver.singularValues().size(); ++i)
        res.singularValues.push_back(solver.singularValues()(i));
    return res;
}

CholeskyResult cholesky(const Matrix& m) {
    checkSquare(m, "Cholesky 分解");
    Eigen::LLT<EMat> llt(toEigen(m));
    if (llt.info() != Eigen::Success)
        throw MkError("矩阵非正定，无法进行 Cholesky 分解");
    return {fromEigen(EMat(llt.matrixL()))};
}

Matrix solve(const Matrix& A, const Matrix& b) {
    checkValid(A);
    checkValid(b);
    if (A.rows != b.rows)
        throw MkError("方程组维度不匹配");
    EMat a = toEigen(A);
    EMat rhs = toEigen(b);
    if (A.rows == A.cols)
        return fromEigen(a.partialPivLu().solve(rhs));
    // 非方阵：最小二乘
    return fromEigen(a.colPivHouseholderQr().solve(rhs));
}

} // namespace mk

#endif // MK_HAVE_EIGEN
