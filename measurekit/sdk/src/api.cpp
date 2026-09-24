// SDK C ABI 薄封装层：只做参数校验、异常到错误码的映射与内存所有权转换，
// 不包含任何业务逻辑（全部委托给 mk_kernel）。
#include "measurekit.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "mk/differentiate.h"
#include "mk/error.h"
#include "mk/evaluator.h"
#include "mk/numerics.h"
#include "mk/parser.h"
#include "mk/printer.h"

namespace {

// 线程局部错误消息存储：error 指针在同一线程的下一次 SDK 调用前有效
thread_local std::string g_last_error;

const char* setError(const std::string& msg) {
    g_last_error = msg;
    return g_last_error.c_str();
}

// 解析表达式为以 var 为自变量的一元函数；解析失败抛 MkError。
// AST 用 shared_ptr 持有，使函数对象可拷贝（newtonRoot 等接口按值传参）。
std::function<double(double)> makeUnaryFn(const char* expr, const char* var) {
    std::shared_ptr<mk::Node> ast(mk::parse(expr).release());
    std::string name(var);
    return [ast, name](double x) {
        mk::Env env;
        env[name] = x;
        return mk::evaluate(*ast, env);
    };
}

// 解析表达式为以 (xvar, yvar) 为自变量的二元函数；解析失败抛 MkError
std::function<double(double, double)> makeBinaryFn(const char* expr,
                                                   const char* xvar,
                                                   const char* yvar) {
    std::shared_ptr<mk::Node> ast(mk::parse(expr).release());
    std::string xname(xvar), yname(yvar);
    return [ast, xname, yname](double x, double y) {
        mk::Env env;
        env[xname] = x;
        env[yname] = y;
        return mk::evaluate(*ast, env);
    };
}

mk_eval_result numericError(const mk::MkError& e) {
    return {MK_ERR_NUMERIC, 0.0, setError(e.what()), e.pos()};
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

namespace {

// 校验表达式与变量绑定参数，合法时填充 env；失败返回非 OK 状态
mk_status bindEnv(const char* expr, const char** names, const double* values,
                  size_t count, mk::Env& env, const char** err) {
    if (!expr) {
        *err = setError("表达式为空");
        return MK_ERR_INVALID_ARG;
    }
    if (count > 0 && (!names || !values)) {
        *err = setError("变量名/值数组为空");
        return MK_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < count; ++i) {
        if (!names[i]) {
            *err = setError("变量名为空");
            return MK_ERR_INVALID_ARG;
        }
        env[names[i]] = values[i];
    }
    return MK_OK;
}

} // namespace

mk_string_result mk_evaluate_steps(const char* expr, const char** names,
                                   const double* values, size_t count) {
    mk::Env env;
    const char* err = nullptr;
    if (bindEnv(expr, names, values, count, env, &err) != MK_OK)
        return {MK_ERR_INVALID_ARG, nullptr, err};

    try {
        mk::NodePtr ast = mk::parse(expr);
        std::vector<mk::EvalStep> steps;
        mk::evaluate(*ast, env, &steps);

        std::string out;
        char line[512];
        for (const auto& s : steps) {
            std::snprintf(line, sizeof(line), "%s = %.12g\n",
                          s.expr.c_str(), s.value);
            out += line;
        }
        char* buf = static_cast<char*>(std::malloc(out.size() + 1));
        if (!buf)
            return {MK_ERR_EVAL, nullptr, setError("内存分配失败")};
        std::memcpy(buf, out.c_str(), out.size() + 1);
        return {MK_OK, buf, nullptr};
    } catch (const mk::MkError& e) {
        return {MK_ERR_EVAL, nullptr, setError(e.what())};
    } catch (const std::exception& e) {
        return {MK_ERR_EVAL, nullptr, setError(e.what())};
    }
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

mk_eval_result mk_newton_root(const char* expr, const char* var,
                              double x0, double tol) {
    if (!expr || !var)
        return {MK_ERR_INVALID_ARG, 0.0, setError("表达式或变量名为空"), 0};

    try {
        auto f = makeUnaryFn(expr, var);
        double root = mk::newtonRoot(f, x0, tol <= 0.0 ? 1e-12 : tol);
        return {MK_OK, root, nullptr, 0};
    } catch (const mk::MkError& e) {
        return numericError(e);
    } catch (const std::exception& e) {
        return {MK_ERR_NUMERIC, 0.0, setError(e.what()), 0};
    }
}

mk_eval_result mk_integrate(const char* expr, const char* var,
                            double a, double b, double tol) {
    if (!expr || !var)
        return {MK_ERR_INVALID_ARG, 0.0, setError("表达式或变量名为空"), 0};

    try {
        auto f = makeUnaryFn(expr, var);
        double v = mk::simpsonIntegral(f, a, b, tol <= 0.0 ? 1e-10 : tol);
        return {MK_OK, v, nullptr, 0};
    } catch (const mk::MkError& e) {
        return numericError(e);
    } catch (const std::exception& e) {
        return {MK_ERR_NUMERIC, 0.0, setError(e.what()), 0};
    }
}

mk_string_result mk_solve_ode(const char* expr, const char* xvar,
                              const char* yvar,
                              double x0, double y0, double h, int steps) {
    if (!expr || !xvar || !yvar)
        return {MK_ERR_INVALID_ARG, nullptr, setError("表达式或变量名为空")};

    try {
        auto f = makeBinaryFn(expr, xvar, yvar);
        auto samples = mk::rungeKutta4(f, x0, y0, h, steps);
        std::string out;
        char line[64];
        for (const auto& s : samples) {
            std::snprintf(line, sizeof(line), "%.17g\t%.17g\n", s.x, s.y);
            out += line;
        }
        char* buf = static_cast<char*>(std::malloc(out.size() + 1));
        if (!buf)
            return {MK_ERR_NUMERIC, nullptr, setError("内存分配失败")};
        std::memcpy(buf, out.c_str(), out.size() + 1);
        return {MK_OK, buf, nullptr};
    } catch (const mk::MkError& e) {
        return {MK_ERR_NUMERIC, nullptr, setError(e.what())};
    } catch (const std::exception& e) {
        return {MK_ERR_NUMERIC, nullptr, setError(e.what())};
    }
}

void mk_free_string(char* s) {
    std::free(s);
}

const char* mk_version(void) {
    return "0.1.0";
}

} // extern "C"
