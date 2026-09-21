#pragma once

#include <QWidget>

#include "mk/linalg.h"

class QTableWidget;
class QSpinBox;
class QComboBox;
class QPlainTextEdit;

// 线性代数标签页：矩阵 A（与矩阵 B）编辑 + 常用运算
class MatrixTab : public QWidget {
    Q_OBJECT
public:
    explicit MatrixTab(QWidget* parent = nullptr);

private slots:
    void run();

private:
    static mk::Matrix readTable(const QTableWidget* table);
    static void fillTable(QTableWidget* table, const mk::Matrix& m);
    static QString formatMatrix(const mk::Matrix& m);

    QTableWidget* aTable_ = nullptr;
    QTableWidget* bTable_ = nullptr;
    QSpinBox* aRows_ = nullptr;
    QSpinBox* aCols_ = nullptr;
    QSpinBox* bRows_ = nullptr;
    QSpinBox* bCols_ = nullptr;
    QComboBox* opCombo_ = nullptr;
    QPlainTextEdit* output_ = nullptr;
};
