// mk_cli：内核冒烟测试 CLI。
// 逐行读入表达式求值；支持 "x=5" 变量赋值（存入 env）；
// "diff <表达式> [变量] [阶数]" 符号求导；输入 quit 退出。
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "mk/differentiate.h"
#include "mk/error.h"
#include "mk/evaluator.h"
#include "mk/parser.h"
#include "mk/printer.h"

namespace {

std::string trim(const std::string& s) {
    std::size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    std::size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool isIdent(const std::string& s) {
    if (s.empty() || !(std::isalpha((unsigned char)s[0]) || s[0] == '_'))
        return false;
    for (char c : s)
        if (!(std::isalnum((unsigned char)c) || c == '_'))
            return false;
    return true;
}

std::string formatNumber(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.12g", v);
    return buf;
}

} // namespace

// diff <表达式> [变量] [阶数]：符号求导并输出化简后的解析式
// 局限：按空白拆分尾部参数，表达式中含空格且以数字/标识符结尾时可能被误解析
static void handleDiff(const std::string& text) {
    std::istringstream iss(text);
    std::vector<std::string> parts;
    for (std::string tok; iss >> tok;)
        parts.push_back(tok);
    if (parts.empty())
        throw mk::MkError("diff 后缺少表达式");

    unsigned order = 1;
    std::string var = "x";
    if (parts.size() >= 2) {
        // 末尾是纯数字 → 阶数
        const std::string& last = parts.back();
        if (!last.empty() && last.find_first_not_of("0123456789") == std::string::npos) {
            order = static_cast<unsigned>(std::strtoul(last.c_str(), nullptr, 10));
            parts.pop_back();
        }
    }
    if (parts.size() >= 2 && isIdent(parts.back())) {
        var = parts.back();
        parts.pop_back();
    }

    std::string expr;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i) expr += ' ';
        expr += parts[i];
    }

    mk::NodePtr ast = mk::parse(expr);
    mk::NodePtr d = order == 1 ? mk::differentiate(*ast, var)
                               : mk::differentiate(*ast, var, order);
    std::cout << mk::toString(*d) << "\n";
}

int main() {
    mk::Env env;
    std::string line;

    while (true) {
        std::cout << ">>> ";
        std::cout.flush();
        if (!std::getline(std::cin, line))
            break;

        std::string input = trim(line);
        if (input.empty())
            continue;
        if (input == "quit")
            break;

        try {
            if (input.rfind("diff ", 0) == 0) {
                handleDiff(trim(input.substr(5)));
                continue;
            }
            // 文法中不含 '='，因此含 '=' 的行按赋值处理
            std::size_t eq = input.find('=');
            if (eq != std::string::npos) {
                std::string name = trim(input.substr(0, eq));
                std::string rhs = trim(input.substr(eq + 1));
                if (!isIdent(name)) {
                    std::cout << "错误：赋值左侧必须是变量名（位置 " << eq << "）\n";
                    continue;
                }
                double v = mk::evaluate(*mk::parse(rhs), env);
                env[name] = v;
                std::cout << name << " = " << formatNumber(v) << "\n";
            } else {
                mk::NodePtr node = mk::parse(input);
                std::cout << formatNumber(mk::evaluate(*node, env)) << "\n";
            }
        } catch (const mk::MkError& e) {
            std::cout << "错误：" << e.what() << "（位置 " << e.pos() << "）\n";
        }
    }
    return 0;
}
