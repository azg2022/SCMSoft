#pragma once

#include <QWidget>

#include <functional>

class QLineEdit;

// 计算器键盘：多套按键布局，点击向输入框插入文本。
// 特殊按键：EVAL=求值、BACK=退格、CLEAR=清空
class CalcKeypad : public QWidget {
    Q_OBJECT
public:
    CalcKeypad(QLineEdit* target, std::function<void()> evaluate, QWidget* parent = nullptr);
};
