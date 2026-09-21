#include "calckeypad.h"

#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <vector>

namespace {

struct Key {
    const char* label;
    const char* insert; // 特殊值：EVAL / BACK / CLEAR
};

// 注意：不能用 std::initializer_list 作为行的存储类型（其底层数组在
// 静态初始化后会悬垂，导致按键文本变成野指针）。std::vector<Key> 会把
// Key 拷贝进自有存储，字符串字面量本身具有静态存储期，二者组合才安全。
using Row = std::vector<Key>;

// 基础布局：数字、四则、括号、百分、幂、等号
const std::vector<Row> kBasic = {
    {{"C", "CLEAR"}, {"⌫", "BACK"}, {"(", "("}, {")", ")"}},
    {{"7", "7"}, {"8", "8"}, {"9", "9"}, {"÷", "/"}},
    {{"4", "4"}, {"5", "5"}, {"6", "6"}, {"×", "*"}},
    {{"1", "1"}, {"2", "2"}, {"3", "3"}, {"−", "-"}},
    {{"0", "0"}, {".", "."}, {"%", "%"}, {"+", "+"}},
    {{"xʸ", "^"}, {"x²", "^2"}, {"√", "sqrt("}, {"π", "pi"}},
    {{"=", "EVAL"}},
};

// 函数布局：常用初等函数
const std::vector<Row> kFunc = {
    {{"sin(", "sin("}, {"cos(", "cos("}, {"tan(", "tan("}, {"ln(", "ln("}},
    {{"log(", "log("}, {"log₂(", "log2("}, {"exp(", "exp("}, {"√", "sqrt("}},
    {{"asin(", "asin("}, {"acos(", "acos("}, {"atan(", "atan("}, {"|x|", "abs("}},
    {{"sinh(", "sinh("}, {"cosh(", "cosh("}, {"tanh(", "tanh("}, {"n!", "!"}},
    {{"Γ(x)", "gamma("}, {"erf(", "erf("}, {"⌊x⌋", "floor("}, {"⌈x⌉", "ceil("}},
    {{"round(", "round("}, {"e", "e"}, {"xʸ", "^"}, {",", ","}},
};

// 变量与常量布局：常用变量字母（配合 x=5 赋值使用）与常量
const std::vector<Row> kConst = {
    {{"π", "pi"}, {"e", "e"}, {"x", "x"}, {"y", "y"}},
    {{"z", "z"}, {"a", "a"}, {"b", "b"}, {"c", "c"}},
    {{"m", "m"}, {"n", "n"}, {"k", "k"}, {"t", "t"}},
    {{"θ", "theta"}, {"α", "alpha"}, {"β", "beta"}, {"λ", "lambda"}},
};

QWidget* buildPad(const std::vector<Row>& rows, QLineEdit* target,
                  const std::function<void()>& eval, QWidget* parent) {
    auto* pad = new QWidget(parent);
    auto* grid = new QGridLayout(pad);
    int r = 0;
    for (const auto& row : rows) {
        int c = 0;
        const int cols = static_cast<int>(row.size());
        for (const auto& k : row) {
            auto* btn = new QPushButton(QString::fromUtf8(k.label), pad);
            QFont f = btn->font();
            f.setPointSize(f.pointSize() + 3);
            btn->setFont(f);
            btn->setMinimumHeight(36);
            const QString ins = QString::fromUtf8(k.insert);
            if (ins == QLatin1String("EVAL"))
                QObject::connect(btn, &QPushButton::clicked, pad, [eval] { eval(); });
            else if (ins == QLatin1String("BACK"))
                QObject::connect(btn, &QPushButton::clicked, pad, [target] { target->backspace(); });
            else if (ins == QLatin1String("CLEAR"))
                QObject::connect(btn, &QPushButton::clicked, pad, [target] { target->clear(); });
            else
                QObject::connect(btn, &QPushButton::clicked, pad, [target, ins] {
                    target->insert(ins);
                    target->setFocus();
                });
            // 整行只有一个按键（如 =）时横向拉满
            if (cols == 1)
                grid->addWidget(btn, r, 0, 1, 4);
            else
                grid->addWidget(btn, r, c);
            ++c;
        }
        ++r;
    }
    return pad;
}

} // namespace

CalcKeypad::CalcKeypad(QLineEdit* target, std::function<void()> evaluate, QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto* tabs = new QTabWidget(this);
    tabs->addTab(buildPad(kBasic, target, evaluate, tabs), tr("基础"));
    tabs->addTab(buildPad(kFunc, target, evaluate, tabs), tr("函数"));
    tabs->addTab(buildPad(kConst, target, evaluate, tabs), tr("变量与常量"));
    layout->addWidget(tabs);

    auto* hint = new QLabel(tr("提示：变量需先赋值再使用，如在输入框键入 x=5 后回车"), this);
    hint->setStyleSheet(QStringLiteral("color: #777;"));
    layout->addWidget(hint);
}
