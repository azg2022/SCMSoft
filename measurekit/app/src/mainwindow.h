#pragma once

#include <QMainWindow>

#include <memory>
#include <unordered_map>

#include "mk/evaluator.h"
#ifdef MK_HAVE_SQLITE
#include "mk/history.h"
#endif

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
    void onClearHistory();
    void onExportHistory();

private:
    QWidget* buildCalcTab();
    QWidget* buildDiffTab();
    void evaluateCurrent();
    // persist=false 仅展示不入库（加载历史时用）
    void appendHistory(const QString& expr, const QString& result, bool persist = true);
    void loadHistory();
    void saveHistory() const;

    mk::Env env_; // 计算标签页中的变量绑定（x=5 赋值累积）
    std::unique_ptr<mk::FormulaEngine> engine_;
#ifdef MK_HAVE_SQLITE
    std::unique_ptr<mk::HistoryStore> historyStore_; // SQLite 持久化，立即写入
#endif

    QLineEdit* exprEdit_ = nullptr;
    QLabel* resultLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QListWidget* stepsList_ = nullptr; // 计算标签页的分步求值过程
    QListWidget* history_ = nullptr;
    QString dataDir_;
};
