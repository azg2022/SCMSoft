#include "matrixtab.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include "mk/error.h"

namespace {

void resizeTable(QTableWidget* table, int rows, int cols) {
    table->setRowCount(rows);
    table->setColumnCount(cols);
    // 新单元格补默认值 0，保留已有内容
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            if (table->item(i, j) == nullptr)
                table->setItem(i, j, new QTableWidgetItem(QStringLiteral("0")));
}

} // namespace

MatrixTab::MatrixTab(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    // 矩阵 A
    auto* groupA = new QGroupBox(tr("矩阵 A"));
    auto* layA = new QVBoxLayout(groupA);
    auto* dimA = new QHBoxLayout;
    aRows_ = new QSpinBox;
    aCols_ = new QSpinBox;
    for (auto* s : {aRows_, aCols_}) {
        s->setRange(1, 12);
        s->setValue(3);
    }
    dimA->addWidget(new QLabel(tr("行:")));
    dimA->addWidget(aRows_);
    dimA->addWidget(new QLabel(tr("列:")));
    dimA->addWidget(aCols_);
    dimA->addStretch();
    layA->addLayout(dimA);
    aTable_ = new QTableWidget(3, 3);
    layA->addWidget(aTable_);
    layout->addWidget(groupA);

    // 矩阵 B（A×B 与解方程组时使用）
    auto* groupB = new QGroupBox(tr("矩阵 B（A×B / 解 Ax=B 时使用）"));
    auto* layB = new QVBoxLayout(groupB);
    auto* dimB = new QHBoxLayout;
    bRows_ = new QSpinBox;
    bCols_ = new QSpinBox;
    for (auto* s : {bRows_, bCols_}) {
        s->setRange(1, 12);
        s->setValue(3);
    }
    dimB->addWidget(new QLabel(tr("行:")));
    dimB->addWidget(bRows_);
    dimB->addWidget(new QLabel(tr("列:")));
    dimB->addWidget(bCols_);
    dimB->addStretch();
    layB->addLayout(dimB);
    bTable_ = new QTableWidget(3, 3);
    layB->addWidget(bTable_);
    layout->addWidget(groupB);

    auto resizeA = [this] { resizeTable(aTable_, aRows_->value(), aCols_->value()); };
    auto resizeB = [this] { resizeTable(bTable_, bRows_->value(), bCols_->value()); };
    connect(aRows_, qOverload<int>(&QSpinBox::valueChanged), this, resizeA);
    connect(aCols_, qOverload<int>(&QSpinBox::valueChanged), this, resizeA);
    connect(bRows_, qOverload<int>(&QSpinBox::valueChanged), this, resizeB);
    connect(bCols_, qOverload<int>(&QSpinBox::valueChanged), this, resizeB);
    resizeA();
    resizeB();

    // 运算选择
    auto* opRow = new QHBoxLayout;
    opCombo_ = new QComboBox;
    opCombo_->addItems({tr("行列式"), tr("转置"), tr("逆"), tr("特征值与特征向量"),
                        tr("L1 范数"), tr("L2 范数（谱范数）"), tr("Frobenius 范数"),
                        tr("∞ 范数"), tr("LU 分解"), tr("QR 分解"), tr("SVD 分解"),
                        tr("Cholesky 分解"), tr("A×B"), tr("解 Ax=B")});
    auto* btn = new QPushButton(tr("计算"));
    opRow->addWidget(opCombo_);
    opRow->addWidget(btn);
    opRow->addStretch();
    layout->addLayout(opRow);

    output_ = new QPlainTextEdit;
    output_->setReadOnly(true);
    output_->setPlaceholderText(tr("结果输出…"));
    layout->addWidget(output_, 1);

    connect(btn, &QPushButton::clicked, this, &MatrixTab::run);
}

mk::Matrix MatrixTab::readTable(const QTableWidget* table) {
    mk::Matrix m;
    m.rows = static_cast<std::size_t>(table->rowCount());
    m.cols = static_cast<std::size_t>(table->columnCount());
    m.data.resize(m.rows * m.cols, 0.0);
    for (int i = 0; i < table->rowCount(); ++i) {
        for (int j = 0; j < table->columnCount(); ++j) {
            QTableWidgetItem* item = table->item(i, j);
            bool ok = false;
            const double v = item ? item->text().trimmed().toDouble(&ok) : 0.0;
            m.data[static_cast<std::size_t>(i) * m.cols + static_cast<std::size_t>(j)] =
                ok ? v : 0.0;
        }
    }
    return m;
}

void MatrixTab::fillTable(QTableWidget* table, const mk::Matrix& m) {
    resizeTable(table, static_cast<int>(m.rows), static_cast<int>(m.cols));
    for (std::size_t i = 0; i < m.rows; ++i)
        for (std::size_t j = 0; j < m.cols; ++j)
            table->item(static_cast<int>(i), static_cast<int>(j))
                ->setText(QString::number(m.data[i * m.cols + j], 'g', 10));
}

QString MatrixTab::formatMatrix(const mk::Matrix& m) {
    QString out;
    for (std::size_t i = 0; i < m.rows; ++i) {
        QStringList row;
        for (std::size_t j = 0; j < m.cols; ++j)
            row << QString::number(m.data[i * m.cols + j], 'g', 8);
        out += row.join(QStringLiteral("  ")) + QLatin1Char('\n');
    }
    return out;
}

void MatrixTab::run() {
    output_->clear();
    try {
        const mk::Matrix A = readTable(aTable_);
        const mk::Matrix B = readTable(bTable_);
        const int op = opCombo_->currentIndex();

        auto print = [this](const QString& s) { output_->appendPlainText(s); };
        auto printM = [this, &print](const QString& title, const mk::Matrix& m) {
            print(title + QStringLiteral(":"));
            print(formatMatrix(m));
        };

        switch (op) {
        case 0: print(QStringLiteral("det(A) = %1").arg(mk::determinant(A))); break;
        case 1: printM(tr("A 的转置"), mk::transpose(A)); break;
        case 2: printM(tr("A 的逆"), mk::inverse(A)); break;
        case 3: {
            const mk::EigenResult r = mk::eigen(A);
            for (std::size_t i = 0; i < r.eigenvalues.size(); ++i) {
                print(QStringLiteral("λ%1 = %2")
                          .arg(i + 1)
                          .arg(r.eigenvalues[i]));
                QStringList vec;
                for (double x : r.eigenvectors[i])
                    vec << QString::number(x, 'g', 8);
                print(QStringLiteral("  v%1 = (%2)").arg(i + 1).arg(vec.join(QStringLiteral(", "))));
            }
            break;
        }
        case 4: print(QStringLiteral("‖A‖₁ = %1").arg(mk::norm(A, mk::NormKind::L1))); break;
        case 5: print(QStringLiteral("‖A‖₂ = %1").arg(mk::norm(A, mk::NormKind::L2))); break;
        case 6: print(QStringLiteral("‖A‖F = %1").arg(mk::norm(A, mk::NormKind::Frobenius))); break;
        case 7: print(QStringLiteral("‖A‖∞ = %1").arg(mk::norm(A, mk::NormKind::Inf))); break;
        case 8: {
            const mk::LUResult r = mk::lu(A);
            printM(tr("L"), r.L);
            printM(tr("U"), r.U);
            printM(tr("P"), r.P);
            break;
        }
        case 9: {
            const mk::QRResult r = mk::qr(A);
            printM(tr("Q"), r.Q);
            printM(tr("R"), r.R);
            break;
        }
        case 10: {
            const mk::SVDResult r = mk::svd(A);
            printM(tr("U"), r.U);
            print(tr("奇异值:"));
            for (double s : r.singularValues)
                print(QStringLiteral("  %1").arg(s));
            printM(tr("V"), r.V);
            break;
        }
        case 11: printM(tr("Cholesky 下三角 L"), mk::cholesky(A).L); break;
        case 12: printM(tr("A×B"), mk::multiply(A, B)); break;
        case 13: printM(tr("x（Ax=B 的解）"), mk::solve(A, B)); break;
        }
    } catch (const mk::MkError& e) {
        output_->setPlainText(tr("错误：%1").arg(QString::fromStdString(e.what())));
    } catch (const std::exception& e) {
        output_->setPlainText(tr("错误：%1").arg(QString::fromStdString(e.what())));
    }
}
