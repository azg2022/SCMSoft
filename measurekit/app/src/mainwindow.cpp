#include "mainwindow.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>

#include "formulapanel.h"
#include "matrixtab.h"
#include "calckeypad.h"

#include "mk/differentiate.h"
#include "mk/error.h"
#include "mk/formula.h"
#include "mk/parser.h"
#include "mk/printer.h"

namespace {

QString formatValue(double v) {
    return QString::number(v, 'g', 12);
}

} // namespace

MainWindow::MainWindow() {
    setWindowTitle(tr("度量衡 MeasureKit"));
    resize(1100, 700);

    dataDir_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir_);

    // 公式库：首次运行播种内置公式
    const QString dbPath = dataDir_ + QStringLiteral("/formulas.db");
    engine_ = std::make_unique<mk::FormulaEngine>(dbPath.toStdString());

    // 种子目录按优先级回退：开发目录 → exe 旁 → 安装目录 → Linux 系统目录
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList seedCandidates = {
        QStringLiteral(MK_SEED_DIR),
        appDir + QStringLiteral("/seed"),
        appDir + QStringLiteral("/../share/measurekit/seed"),
        QStringLiteral("/usr/share/measurekit/seed"),
    };
    if (engine_->listByCategory(QStringLiteral("general").toStdString()).empty()) {
        for (const QString& dir : seedCandidates) {
            if (QDir(dir).exists()) {
                try {
                    engine_->seedFromJsonDir(dir.toStdString());
                } catch (const std::exception& e) {
                    QMessageBox::warning(this, tr("公式库加载失败"),
                                         tr("内置公式加载失败：%1").arg(QString::fromStdString(e.what())));
                }
                break;
            }
        }
    }

    auto* tabs = new QTabWidget;
    tabs->addTab(buildCalcTab(), tr("计算"));
    tabs->addTab(buildDiffTab(), tr("符号求导"));
    auto* matrixTab = new MatrixTab;
#ifdef MK_HAVE_EIGEN
    tabs->addTab(matrixTab, tr("线性代数"));
#else
    delete matrixTab;
#endif
    setCentralWidget(tabs);

    // 左侧：公式库面板
    auto* formulaDock = new QDockWidget(tr("公式库"), this);
    formulaDock->setObjectName(QStringLiteral("formulaDock"));
    auto* panel = new FormulaPanel(engine_.get());
    formulaDock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea, formulaDock);
    connect(panel, &FormulaPanel::historyRequested, this,
            [this](const QString& expr, const QString& result) { appendHistory(expr, result); });

    // 右侧：历史记录
    auto* historyDock = new QDockWidget(tr("历史记录"), this);
    historyDock->setObjectName(QStringLiteral("historyDock"));
    history_ = new QListWidget;
    historyDock->setWidget(history_);
    addDockWidget(Qt::RightDockWidgetArea, historyDock);
    connect(history_, &QListWidget::itemDoubleClicked, this,
            &MainWindow::onHistoryDoubleClicked);

    // 菜单栏：视图（可重新打开被关闭的面板）+ 帮助
    QMenu* viewMenu = menuBar()->addMenu(tr("视图"));
    viewMenu->addAction(formulaDock->toggleViewAction());
    viewMenu->addAction(historyDock->toggleViewAction());
    QMenu* helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(tr("关于 MeasureKit"), this, [this] {
        QMessageBox::about(this, tr("关于 MeasureKit"),
                           tr("度量衡 MeasureKit 0.1.0\n"
                              "智能科学计算器：表达式计算 · 符号求导 · 线性代数 · 公式库\n"
                              "内核：C++17 ｜ 界面：Qt 6"));
    });

    // 工具栏：面板开关 + 清空历史
    QToolBar* toolBar = addToolBar(tr("主工具栏"));
    toolBar->setObjectName(QStringLiteral("mainToolBar"));
    toolBar->addAction(formulaDock->toggleViewAction());
    toolBar->addAction(historyDock->toggleViewAction());
    toolBar->addSeparator();
    toolBar->addAction(tr("清空历史"), this, [this] { history_->clear(); });

    loadHistory();
}

MainWindow::~MainWindow() = default;

QWidget* MainWindow::buildCalcTab() {
    auto* tab = new QWidget;
    auto* layout = new QVBoxLayout(tab);

    exprEdit_ = new QLineEdit;
    exprEdit_->setPlaceholderText(tr("输入表达式，如 2+3*4、sin(pi/2)、x^2+2x-1；或赋值如 x=5"));
    QFont editFont = exprEdit_->font();
    editFont.setPointSize(editFont.pointSize() + 4);
    exprEdit_->setFont(editFont);
    layout->addWidget(exprEdit_);

    resultLabel_ = new QLabel(tr("—"));
    QFont resultFont = resultLabel_->font();
    resultFont.setPointSize(resultFont.pointSize() + 10);
    resultFont.setBold(true);
    resultLabel_->setFont(resultFont);
    resultLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(resultLabel_);

    statusLabel_ = new QLabel;
    statusLabel_->setStyleSheet(QStringLiteral("color: #c0392b;"));
    layout->addWidget(statusLabel_);

    // 分步求值过程（后序，最后一行是完整表达式与最终结果）
    stepsList_ = new QListWidget;
    stepsList_->setMaximumHeight(140);
    stepsList_->setStyleSheet(QStringLiteral(
        "QListWidget { background: #fafafa; border: 1px solid #e0e0e0; }"));
    layout->addWidget(stepsList_);

    // 计算器键盘（基础/函数/变量三套布局），填充下方空间
    layout->addWidget(new CalcKeypad(exprEdit_, [this] { evaluateCurrent(); }), 1);

    connect(exprEdit_, &QLineEdit::textChanged, this, &MainWindow::onExprEdited);
    connect(exprEdit_, &QLineEdit::returnPressed, this, &MainWindow::onExprReturn);
    return tab;
}

QWidget* MainWindow::buildDiffTab() {
    auto* tab = new QWidget;
    auto* layout = new QFormLayout(tab);

    auto* exprEdit = new QLineEdit;
    exprEdit->setPlaceholderText(tr("输入表达式，如 x*sin(x)"));
    auto* varEdit = new QLineEdit(QStringLiteral("x"));
    varEdit->setMaximumWidth(80);
    auto* orderSpin = new QSpinBox;
    orderSpin->setRange(1, 10);
    orderSpin->setValue(1);
    auto* btn = new QPushButton(tr("求导"));
    auto* result = new QLabel(tr("—"));
    result->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont f = result->font();
    f.setPointSize(f.pointSize() + 6);
    f.setBold(true);
    result->setFont(f);
    auto* error = new QLabel;
    error->setStyleSheet(QStringLiteral("color: #c0392b;"));

    layout->addRow(tr("表达式"), exprEdit);
    layout->addRow(tr("变量"), varEdit);
    layout->addRow(tr("阶数"), orderSpin);
    layout->addRow(QString(), btn);
    layout->addRow(tr("结果"), result);
    layout->addRow(QString(), error);

    auto run = [this, exprEdit, varEdit, orderSpin, result, error] {
        error->clear();
        try {
            auto ast = mk::parse(exprEdit->text().trimmed().toStdString());
            auto d = mk::differentiate(*ast, varEdit->text().trimmed().toStdString(),
                                       static_cast<unsigned>(orderSpin->value()));
            result->setText(QString::fromStdString(mk::toString(*d)));
        } catch (const mk::MkError& e) {
            result->setText(tr("错误"));
            error->setText(QString::fromStdString(e.what()));
        } catch (const std::exception& e) {
            result->setText(tr("错误"));
            error->setText(QString::fromStdString(e.what()));
        }
    };
    connect(btn, &QPushButton::clicked, this, run);
    connect(exprEdit, &QLineEdit::returnPressed, this, run);
    return tab;
}

void MainWindow::onExprEdited(const QString& text) {
    if (text.trimmed().isEmpty()) {
        exprEdit_->setStyleSheet(QString());
        statusLabel_->clear();
        resultLabel_->setText(tr("—"));
        stepsList_->clear();
        return;
    }
    // 赋值形式只校验等号右侧
    QString expr = text.trimmed();
    const int eq = expr.indexOf(QLatin1Char('='));
    if (eq > 0)
        expr = expr.mid(eq + 1).trimmed();
    if (expr.isEmpty())
        return;
    try {
        mk::parse(expr.toStdString());
        exprEdit_->setStyleSheet(QString());
        statusLabel_->clear();
    } catch (const mk::MkError& e) {
        exprEdit_->setStyleSheet(QStringLiteral("border: 1px solid #c0392b;"));
        statusLabel_->setText(tr("第 %1 个字符：%2")
                                  .arg(e.pos() + 1)
                                  .arg(QString::fromStdString(e.what())));
    }
}

void MainWindow::onExprReturn() {
    evaluateCurrent();
}

void MainWindow::evaluateCurrent() {
    const QString text = exprEdit_->text().trimmed();
    if (text.isEmpty())
        return;

    static const QRegularExpression identRe(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    const int eq = text.indexOf(QLatin1Char('='));
    try {
        if (eq > 0) { // 赋值：x=5
            const QString name = text.left(eq).trimmed();
            if (!identRe.match(name).hasMatch())
                throw mk::MkError("赋值左侧必须是合法变量名");
            const QString rhs = text.mid(eq + 1).trimmed();
            auto ast = mk::parse(rhs.toStdString());
            const double v = mk::evaluate(*ast, env_);
            env_[name.toStdString()] = v;
            resultLabel_->setText(name + QStringLiteral(" = ") + formatValue(v));
            statusLabel_->clear();
            exprEdit_->setStyleSheet(QString());
            stepsList_->clear();
            appendHistory(text, formatValue(v));
            return;
        }

        auto ast = mk::parse(text.toStdString());
        std::vector<mk::EvalStep> steps;
        const double v = mk::evaluate(*ast, env_, &steps);
        resultLabel_->setText(formatValue(v));
        statusLabel_->clear();
        exprEdit_->setStyleSheet(QString());
        stepsList_->clear();
        for (const auto& s : steps) {
            auto* item = new QListWidgetItem(QString::fromStdString(s.expr) +
                                             QStringLiteral("  =  ") + formatValue(s.value));
            stepsList_->addItem(item);
        }
        appendHistory(text, formatValue(v));
    } catch (const mk::MkError& e) {
        resultLabel_->setText(tr("错误"));
        statusLabel_->setText(QString::fromStdString(e.what()));
        stepsList_->clear();
    } catch (const std::exception& e) {
        resultLabel_->setText(tr("错误"));
        statusLabel_->setText(QString::fromStdString(e.what()));
        stepsList_->clear();
    }
}

void MainWindow::appendHistory(const QString& expr, const QString& result) {
    auto* item = new QListWidgetItem(expr + QStringLiteral("  =  ") + result);
    item->setData(Qt::UserRole, expr);
    item->setToolTip(expr);
    history_->insertItem(0, item);
}

void MainWindow::onHistoryDoubleClicked(QListWidgetItem* item) {
    if (item)
        exprEdit_->setText(item->data(Qt::UserRole).toString());
}

void MainWindow::loadHistory() {
    QFile f(dataDir_ + QStringLiteral("/history.txt"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const int tab = line.indexOf(QLatin1Char('\t'));
        if (tab < 0)
            continue;
        appendHistory(line.left(tab), line.mid(tab + 1));
    }
}

void MainWindow::saveHistory() const {
    QFile f(dataDir_ + QStringLiteral("/history.txt"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&f);
    for (int i = 0; i < history_->count(); ++i) {
        const QListWidgetItem* item = history_->item(i);
        out << item->data(Qt::UserRole).toString() << QLatin1Char('\t')
            << item->text().section(QStringLiteral("  =  "), 1) << QLatin1Char('\n');
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveHistory();
    QMainWindow::closeEvent(event);
}
