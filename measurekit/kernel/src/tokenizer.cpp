#include "mk/tokenizer.h"

#include <cctype>
#include <cstdlib>

#include "mk/error.h"

namespace mk {

static bool isIdentStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool isIdentChar(char c) {
    return isIdentStart(c) || (c >= '0' && c <= '9') || c == '_';
}

std::vector<Token> tokenize(const std::string& src) {
    std::vector<Token> toks;
    std::size_t i = 0;
    const std::size_t n = src.size();

    while (i < n) {
        char c = src[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            ++i;
            continue;
        }

        // 数字：整数/小数/科学计数。'e'/'E' 仅在后面跟数字或 [+-]数字 时视为指数，
        // 否则留给 Ident（如欧拉常数 e 或变量名 e2）。
        if ((c >= '0' && c <= '9') ||
            (c == '.' && i + 1 < n && src[i + 1] >= '0' && src[i + 1] <= '9')) {
            std::size_t start = i;
            while (i < n && src[i] >= '0' && src[i] <= '9') ++i;
            if (i < n && src[i] == '.') {
                ++i;
                while (i < n && src[i] >= '0' && src[i] <= '9') ++i;
            }
            if (i < n && (src[i] == 'e' || src[i] == 'E')) {
                std::size_t j = i + 1;
                if (j < n && (src[j] == '+' || src[j] == '-')) ++j;
                if (j < n && src[j] >= '0' && src[j] <= '9') {
                    i = j;
                    while (i < n && src[i] >= '0' && src[i] <= '9') ++i;
                }
            }
            std::string text = src.substr(start, i - start);
            toks.push_back({TokKind::Number, std::strtod(text.c_str(), nullptr), text, start});
            continue;
        }

        if (isIdentStart(c)) {
            std::size_t start = i;
            while (i < n && isIdentChar(src[i])) ++i;
            toks.push_back({TokKind::Ident, 0, src.substr(start, i - start), start});
            continue;
        }

        switch (c) {
        case '+': case '-': case '*': case '/': case '^': case '!': case '%':
            toks.push_back({TokKind::Op, 0, std::string(1, c), i});
            ++i;
            break;
        case '(':
            toks.push_back({TokKind::LParen, 0, "(", i});
            ++i;
            break;
        case ')':
            toks.push_back({TokKind::RParen, 0, ")", i});
            ++i;
            break;
        case ',':
            toks.push_back({TokKind::Comma, 0, ",", i});
            ++i;
            break;
        default:
            throw MkError(std::string("不支持的字符 '") + c + "'", i);
        }
    }

    toks.push_back({TokKind::End, 0, "", n});
    return toks;
}

} // namespace mk
