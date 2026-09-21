#include "mk/parser.h"

#include "mk/error.h"
#include "mk/tokenizer.h"

namespace mk {

// 递归下降解析器。文法（优先级从低到高）：
//   expr    := term (('+'|'-') term)*
//   term    := unary (('*'|'/'|隐式乘法) unary)*
//   unary   := ('-'|'+') unary | power
//   power   := postfix ('^' unary)?        // 右结合
//   postfix := primary ('!'|'%')*
//   primary := Number | Ident ['(' expr (',' expr)* ')'] | '(' expr ')'
class Parser {
public:
    explicit Parser(std::vector<Token> toks) : toks_(std::move(toks)) {}

    NodePtr run() {
        if (peek().kind == TokKind::End)
            throw MkError("表达式为空", peek().pos);
        NodePtr node = parseExpr();
        if (peek().kind != TokKind::End)
            throw MkError("表达式末尾有多余内容", peek().pos);
        return node;
    }

private:
    const Token& peek() const { return toks_[pos_]; }
    const Token& advance() { return toks_[pos_++]; }

    bool isOp(char c) const {
        return peek().kind == TokKind::Op && peek().text[0] == c;
    }

    NodePtr parseExpr() {
        NodePtr lhs = parseTerm();
        while (peek().kind == TokKind::Op && (isOp('+') || isOp('-'))) {
            char op = advance().text[0];
            NodePtr rhs = parseTerm();
            lhs = binary(op, std::move(lhs), std::move(rhs));
        }
        return lhs;
    }

    NodePtr parseTerm() {
        NodePtr lhs = parseUnary();
        for (;;) {
            if (isOp('*') || isOp('/')) {
                char op = advance().text[0];
                NodePtr rhs = parseUnary();
                lhs = binary(op, std::move(lhs), std::move(rhs));
            } else if (peek().kind == TokKind::Number ||
                       peek().kind == TokKind::Ident ||
                       peek().kind == TokKind::LParen) {
                // 隐式乘法：2x、2(x+1)、x sin(y)
                NodePtr rhs = parseUnary();
                lhs = binary('*', std::move(lhs), std::move(rhs));
            } else {
                break;
            }
        }
        return lhs;
    }

    NodePtr parseUnary() {
        if (isOp('-')) {
            advance();
            return unary('-', parseUnary());
        }
        if (isOp('+')) {
            advance();
            return parseUnary(); // 一元正号不改变语义，直接透传
        }
        return parsePower();
    }

    NodePtr parsePower() {
        NodePtr base = parsePostfix();
        if (isOp('^')) {
            advance();
            // 右结合，且指数可带一元符号：2^3^2、2^-3
            NodePtr exp = parseUnary();
            return binary('^', std::move(base), std::move(exp));
        }
        return base;
    }

    NodePtr parsePostfix() {
        NodePtr node = parsePrimary();
        while (isOp('!') || isOp('%')) {
            char op = advance().text[0];
            node = unary(op, std::move(node));
        }
        return node;
    }

    NodePtr parsePrimary() {
        const Token& t = peek();
        switch (t.kind) {
        case TokKind::Number:
            advance();
            return num(t.num);
        case TokKind::Ident: {
            advance();
            std::string name = t.text;
            if (peek().kind == TokKind::LParen) {
                // Ident 后紧跟 '(' 才是函数调用，否则是变量
                advance();
                std::vector<NodePtr> args;
                if (peek().kind != TokKind::RParen) {
                    args.push_back(parseExpr());
                    while (peek().kind == TokKind::Comma) {
                        advance();
                        args.push_back(parseExpr());
                    }
                }
                if (peek().kind != TokKind::RParen)
                    throw MkError("函数调用缺少右括号 ')'", peek().pos);
                advance();
                return call(name, std::move(args));
            }
            return var(name);
        }
        case TokKind::LParen: {
            advance();
            NodePtr inner = parseExpr();
            if (peek().kind != TokKind::RParen)
                throw MkError("缺少右括号 ')'", peek().pos);
            advance();
            return inner;
        }
        case TokKind::End:
            throw MkError("表达式意外结束", t.pos);
        default:
            throw MkError("此处应为数字、变量或 '('", t.pos);
        }
    }

    std::vector<Token> toks_;
    std::size_t pos_ = 0;
};

NodePtr parse(const std::string& src) {
    Parser p(tokenize(src));
    return p.run();
}

} // namespace mk
