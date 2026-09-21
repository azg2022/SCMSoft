#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace mk {

enum class TokKind { Number, Ident, Op, LParen, RParen, Comma, End };

struct Token {
    TokKind kind;
    double num = 0;        // Kind == Number 时的数值
    std::string text;      // Ident 名称 / Op 字符 / Number 原文
    std::size_t pos = 0;   // token 起始字符位置
};

// 词法分析；遇到非法字符抛 MkError("不支持的字符 'X'", pos)
std::vector<Token> tokenize(const std::string& src);

} // namespace mk
