#include "formulapanel.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>

#include "mk/error.h"

namespace {

// 分类键 → 显示名
const std::vector<std::pair<const char*, const char*>> kCategories = {
    {"general", "一般数学"},
    {"probability", "概率统计"},
    {"statistics", "数据分析"},
    {"linalg", "线性代数"},
    {"calculus", "微积分"},
};

QString categoryName(const QString& key) {
    for (const auto& [k, name] : kCategories)
        if (key == QLatin1String(k))
            return QString::fromUtf8(name);
    return key;
}

} // namespace

FormulaPanel::FormulaPanel(mk::FormulaEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    auto* layout = new QVBoxLayout(this);

    search_ = new QLineEdit;
    search_->setPlaceholderText(tr("搜索公式名称…"));
    layout->addWidget(search_);

    auto* splitter = new QSplitter(Qt::Vertical);
    categoryList_ = new QListWidget;
    categoryList_->setMaximumHeight(150);
    formulaList_ = new QListWidget;
    splitter->addWidget(categoryList_);
    splitter->addWidget(formulaList_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    infoLabel_ = new QLabel;
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet(QStringLiteral("color: #555;"));

    // 参数表单放在滚动区；与公式列表之间用分割条隔开，可上下拖拽调整高度
    auto* paramContainer = new QWidget;
    paramForm_ = new QFormLayout(paramContainer);
    paramForm_->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea;
    scroll->setWidget(paramContainer);
    scroll->setWidgetResizable(true);

    auto* bottom = new QWidget;
    auto* bottomLayout = new QVBoxLayout(bottom);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->addWidget(infoLabel_);
    bottomLayout->addWidget(scroll, 1);

    auto* mainSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(splitter);
    mainSplitter->addWidget(bottom);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);
    layout->addWidget(mainSplitter, 1);

    auto* btn = new QPushButton(tr("计算"));
    layout->addWidget(btn);

    resultLabel_ = new QLabel(tr("—"));
    QFont f = resultLabel_->font();
    f.setBold(true);
    f.setPointSize(f.pointSize() + 4);
    resultLabel_->setFont(f);
    resultLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(resultLabel_);

    errorLabel_ = new QLabel;
    errorLabel_->setStyleSheet(QStringLiteral("color: #c0392b;"));
    errorLabel_->setWordWrap(true);
    layout->addWidget(errorLabel_);

    connect(search_, &QLineEdit::textChanged, this, &FormulaPanel::onSearchChanged);
    connect(categoryList_, &QListWidget::currentRowChanged, this,
            [this](int) { onCategoryChanged(); });
    connect(formulaList_, &QListWidget::currentRowChanged, this,
            [this](int) { onFormulaSelected(); });
    connect(btn, &QPushButton::clicked, this, &FormulaPanel::compute);

    reloadCategories();
}

void FormulaPanel::reloadCategories() {
    categoryList_->clear();
    categoryList_->addItem(tr("搜索结果/全部"));
    for (const auto& [key, name] : kCategories) {
        auto* item = new QListWidgetItem(QString::fromUtf8(name));
        item->setData(Qt::UserRole, QString::fromUtf8(key));
        categoryList_->addItem(item);
    }
    categoryList_->setCurrentRow(0);
}

void FormulaPanel::onSearchChanged(const QString&) {
    // 搜索模式下展示全部命中的公式
    if (!search_->text().trimmed().isEmpty())
        categoryList_->setCurrentRow(0);
    reloadFormulas();
    onCategoryChanged();
}

void FormulaPanel::onCategoryChanged() {
    reloadFormulas();
}

void FormulaPanel::reloadFormulas() {
    formulaList_->clear();
    current_.reset();
    rebuildParamForm();
    infoLabel_->clear();

    std::vector<mk::Formula> formulas;
    const QString keyword = search_->text().trimmed();
    QListWidgetItem* catItem = categoryList_->currentItem();
    if (!keyword.isEmpty()) {
        formulas = engine_->search(keyword.toStdString());
    } else if (catItem && catItem->data(Qt::UserRole).isValid()) {
        formulas = engine_->listByCategory(catItem->data(Qt::UserRole).toString().toStdString());
    } else {
        for (const auto& [key, name] : kCategories) {
            auto part = engine_->listByCategory(key);
            formulas.insert(formulas.end(), part.begin(), part.end());
        }
    }

    for (const auto& f : formulas) {
        auto* item = new QListWidgetItem(QString::fromStdString(f.name));
        item->setData(Qt::UserRole, static_cast<qlonglong>(f.id));
        item->setToolTip(categoryName(QString::fromStdString(f.category)));
        formulaList_->addItem(item);
    }
}

void FormulaPanel::onFormulaSelected() {
    QListWidgetItem* item = formulaList_->currentItem();
    current_.reset();
    rebuildParamForm();
    infoLabel_->clear();
    if (!item) {
        resultLabel_->setText(tr("—"));
        return;
    }
    try {
        current_ = engine_->get(item->data(Qt::UserRole).toLongLong());
        const mk::Formula& f = *current_;
        infoLabel_->setText(QStringLiteral("%1 ｜ %2")
                                .arg(categoryName(QString::fromStdString(f.category)))
                                .arg(QString::fromStdString(f.latex)));
        rebuildParamForm();
    } catch (const mk::MkError& e) {
        showError(QString::fromStdString(e.what()));
    }
}

void FormulaPanel::rebuildParamForm() {
    while (paramForm_->rowCount() > 0)
        paramForm_->removeRow(0);
    paramInputs_.clear();
    if (!current_)
        return;
    for (const auto& p : current_->params) {
        auto* spin = new QDoubleSpinBox;
        spin->setRange(-1e15, 1e15);
        spin->setDecimals(10);
        spin->setValue(p.defaultValue);
        QString label = QString::fromStdString(p.symbol);
        if (!p.label.empty())
            label += QStringLiteral("（%1）").arg(QString::fromStdString(p.label));
        if (!p.unit.empty())
            label += QStringLiteral(" [%1]").arg(QString::fromStdString(p.unit));
        if (!p.domain.empty())
            label += QStringLiteral(" 定义域:%1").arg(QString::fromStdString(p.domain));
        paramForm_->addRow(label, spin);
        paramInputs_.push_back(spin);
    }
}

void FormulaPanel::compute() {
    errorLabel_->clear();
    if (!current_)
        return;
    std::unordered_map<std::string, double> bindings;
    const auto& params = current_->params;
    for (std::size_t i = 0; i < paramInputs_.size() && i < params.size(); ++i)
        bindings[params[i].symbol] = paramInputs_[i]->value();
    try {
        const double v = engine_->evaluate(*current_, bindings);
        resultLabel_->setText(QString::number(v, 'g', 12));
        emit historyRequested(QString::fromStdString(current_->name),
                              QString::number(v, 'g', 12));
    } catch (const mk::MkError& e) {
        showError(QString::fromStdString(e.what()));
    } catch (const std::exception& e) {
        showError(QString::fromStdString(e.what()));
    }
}

void FormulaPanel::showError(const QString& message) {
    resultLabel_->setText(tr("错误"));
    errorLabel_->setText(message);
}
