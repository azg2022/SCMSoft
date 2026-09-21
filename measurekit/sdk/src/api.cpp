// SDK C ABI 薄封装层：只做参数校验、异常到错误码的映射与内存所有权转换，
// 不包含任何业务逻辑（全部委托给 mk_kernel）。
#include "measurekit.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

#include "mk/differentiate.h"
#include "mk/error.h"
#include "mk/evaluator.h"
#include "mk/parser.h"
#include "mk/printer.h"

namespace {

// 线程局部错误消息存储：error 指针在同一线程的下一次 SDK 调用前有效
thread_local std::string g_last_error;

const char* setError(const std::string& msg) {
    g_last_error = msg;
    return g_last_error.c_str();
}

} // namespace

extern "C" {

mk_eval_result mk_evaluate_with(const char* expr, const char** names,
                                const double* values, size_t count) {
    if (!expr)
        return {MK_ERR_INVALID_ARG, 0.0, setError("表达式为空"), 0};
    if (count > 0 && (!names || !values))
        return {MK_ERR_INVALID_ARG, 0.0, setError("变量名/值数组为空"), 0};
    for (size_t i = 0; i < count; ++i)
        if (!names[i])
            return {MK_ERR_INVALID_ARG, 0.0, setError("变量名为空"), 0};

    mk::NodePtr ast;
    try {
        ast = mk::parse(expr); // 语法错误 → MK_ERR_PARSE，位置透传
    } catch (const mk::MkError& e) {
        return {MK_ERR_PARSE, 0.0, setError(e.what()), e.pos()};
    } catch (const std::exception& e) {
        return {MK_ERR_PARSE, 0.0, setError(e.what()), 0};
    }

    try {
        mk::Env env;
        for (size_t i = 0; i < count; ++i)
            env[names[i]] = values[i];
        double v = mk::evaluate(*ast, env);
        return {MK_OK, v, nullptr, 0};
    } catch (const mk::MkError& e) {
        return {MK_ERR_EVAL, 0.0, setError(e.what()), e.pos()};
    } catch (const std::exception& e) {
        return {MK_ERR_EVAL, 0.0, setError(e.what()), 0};
    }
}

mk_eval_result mk_evaluate(const char* expr) {
    return mk_evaluate_with(expr, nullptr, nullptr, 0);
}

mk_string_result mk_differentiate(const char* expr, const char* var,
                                  unsigned order) {
    if (!expr || !var)
        return {MK_ERR_INVALID_ARG, nullptr, setError("表达式或变量名为空")};
    if (order < 1)
        return {MK_ERR_INVALID_ARG, nullptr, setError("求导阶数必须 ≥ 1")};

    try {
        mk::NodePtr ast = mk::parse(expr);
        mk::NodePtr d = order == 1 ? mk::differentiate(*ast, var)
                                   : mk::differentiate(*ast, var, order);
        std::string s = mk::toString(*d);
        char* out = static_cast<char*>(std::malloc(s.size() + 1));
        if (!out)
            return {MK_ERR_DIFF, nullptr, setError("内存分配失败")};
        std::memcpy(out, s.c_str(), s.size() + 1);
        return {MK_OK, out, nullptr};
    } catch (const mk::MkError& e) {
        return {MK_ERR_DIFF, nullptr, setError(e.what())};
    } catch (const std::exception& e) {
        return {MK_ERR_DIFF, nullptr, setError(e.what())};
    }
}

void mk_free_string(char* s) {
    std::free(s);
}

const char* mk_version(void) {
    return "0.1.0";
}

} // extern "C"
