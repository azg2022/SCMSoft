/*
 * measurekit.h —— 度量衡 MeasureKit 内核的 C ABI 薄封装层
 *
 * ABI 冻结约定：本头文件发布后只增不改——只允许追加新函数/新枚举值，
 * 不得修改或删除已有函数的签名、结构体布局和枚举取值。
 *
 * 内存所有权规则：
 *   - mk_string_result.text 归调用方所有，必须调用 mk_free_string 释放；
 *   - 所有结果中的 error 指针指向 SDK 的线程局部存储，调用方不得释放，
 *     同一线程的下一次 SDK 调用会使其失效；
 *   - 输入参数（expr / names / values）均为只读借用，SDK 不接管所有权。
 */
#ifndef MEASUREKIT_H
#define MEASUREKIT_H

#include <stddef.h>

#if defined(_WIN32)
#define MK_API __declspec(dllexport)
#else
#define MK_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 错误码：0 = 成功 */
typedef enum mk_status {
    MK_OK = 0,
    MK_ERR_PARSE = 1,      /* 语法错误 */
    MK_ERR_EVAL = 2,       /* 求值/定义域错误 */
    MK_ERR_DIFF = 3,       /* 求导错误 */
    MK_ERR_INVALID_ARG = 4 /* 参数为空等 */
} mk_status;

/* 求值结果：调用方必须检查 status */
typedef struct mk_eval_result {
    mk_status status;
    double value;
    const char* error;   /* NULL 表示无错误；否则指向线程局部错误消息 */
    size_t error_pos;    /* 语法错误的字符位置，无错误时为 0 */
} mk_eval_result;

/* 字符串结果（求导输出解析式）：text 由 mk_free_string 释放 */
typedef struct mk_string_result {
    mk_status status;
    char* text;          /* 调用方用 mk_free_string 释放；失败时为 NULL */
    const char* error;
} mk_string_result;

/* 无变量求值，内置常量 pi/e */
MK_API mk_eval_result mk_evaluate(const char* expr);

/* 带变量绑定求值；names/values 长度均为 count */
MK_API mk_eval_result mk_evaluate_with(const char* expr,
                                       const char** names, const double* values,
                                       size_t count);

/* 符号求导（对 var 求 order 阶导，order >= 1），输出化简后的解析式 */
MK_API mk_string_result mk_differentiate(const char* expr, const char* var,
                                         unsigned order);

/* 释放 mk_string_result.text */
MK_API void mk_free_string(char* s);

/* 版本号，当前 "0.1.0" */
MK_API const char* mk_version(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MEASUREKIT_H */
