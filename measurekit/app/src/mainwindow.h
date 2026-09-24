#pragma once

#include <QMainWindow>

#include <memory>
#include <unordered_map>

#include "mk/evaluator.h"

class QLineEdit;
class QLabel;
class QListWidget;
class QListWidgetItem;
class FormulaPanel;
class MatrixTab;

namespace mk {
class FormulaEngine;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onExprEdited(const QString& text);
    void onExprReturn();
    void onHistoryDoubleClicked(QListWidgetItem* item);

private:
    QWidget* buildCalcTab();
    QWidget* buildDiffTab();
    void evaluateCurrent();
    void appendHistory(const QString& expr, const QString& result);
    void loadHistory();
    void saveHistory() const;

    mk::Env env_; // 计算标签页中的变量绑定（x=5 赋值累积）
    std::unique_ptr<mk::FormulaEngine> engine_;

    QLineEdit* exprEdit_ = nullptr;
    QLabel* resultLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QListWidget* stepsList_ = nullptr; // 计算标签页的分步求值过程
    QListWidget* history_ = nullptr;
    QString dataDir_;
};
