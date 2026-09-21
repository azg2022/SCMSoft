#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace mk {

// 错误信息：message 为用户可读的中文描述，pos 为出错处的字符位置
struct Error {
    std::string message;
    std::size_t pos = 0;
};

// 词法/语法/求值统一抛出的异常类型，消息使用中文
class MkError : public std::runtime_error {
public:
    explicit MkError(std::string msg, std::size_t pos = 0)
        : std::runtime_error(std::move(msg)), pos_(pos) {}

    std::size_t pos() const { return pos_; }

private:
    std::size_t pos_;
};

} // namespace mk
