#pragma once

#include <QWidget>

#include <optional>
#include <vector>

#include "mk/formula.h"

class QLineEdit;
class QListWidget;
class QFormLayout;
class QLabel;
class QDoubleSpinBox;

// 公式库面板：分类浏览 + 搜索 + 参数绑定求值
class FormulaPanel : public QWidget {
    Q_OBJECT
public:
    explicit FormulaPanel(mk::FormulaEngine* engine, QWidget* parent = nullptr);

signals:
    // 求值成功，请求主窗口记入历史
    void historyRequested(const QString& expr, const QString& result);

private slots:
    void onSearchChanged(const QString& text);
    void onCategoryChanged();
    void onFormulaSelected();
    void compute();

private:
    void reloadCategories();
    void reloadFormulas();
    void rebuildParamForm();
    void showError(const QString& message);

    mk::FormulaEngine* engine_;
    QLineEdit* search_;
    QListWidget* categoryList_;
    QListWidget* formulaList_;
    QLabel* infoLabel_;
    QFormLayout* paramForm_;
    std::vector<QDoubleSpinBox*> paramInputs_;
    std::optional<mk::Formula> current_;
    QLabel* resultLabel_;
    QLabel* errorLabel_;
};
