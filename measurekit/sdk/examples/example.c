/* mk_sdk_example：纯 C 调用方示例，验证 C ABI 层可用。
 * 校验返回值是否正确，全部通过返回 0，否则返回 1 并打印失败项。 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "measurekit.h"

static int failures = 0;

static void check(int ok, const char* what) {
    if (!ok) {
        printf("FAIL: %s\n", what);
        ++failures;
    }
}

int main(void) {
    printf("measurekit SDK %s\n", mk_version());

    /* 无变量求值：2+3*4 = 14 */
    mk_eval_result r1 = mk_evaluate("2+3*4");
    check(r1.status == MK_OK && fabs(r1.value - 14.0) < 1e-12, "mk_evaluate");
    printf("2+3*4 = %g\n", r1.value);

    /* 带变量绑定：x=5 时 x^2+1 = 26 */
    const char* names[] = {"x"};
    double values[] = {5.0};
    mk_eval_result r2 = mk_evaluate_with("x^2+1", names, values, 1);
    check(r2.status == MK_OK && fabs(r2.value - 26.0) < 1e-12, "mk_evaluate_with");
    printf("x^2+1 (x=5) = %g\n", r2.value);

    /* 符号求导：d/dx[x*sin(x)] = sin(x)+x*cos(x) */
    mk_string_result r3 = mk_differentiate("x*sin(x)", "x", 1);
    check(r3.status == MK_OK && r3.text && strcmp(r3.text, "sin(x)+x*cos(x)") == 0,
          "mk_differentiate");
    printf("d/dx x*sin(x) = %s\n", r3.text ? r3.text : "(null)");
    mk_free_string(r3.text);

    /* 错误路径：语法错误 → MK_ERR_PARSE 且带位置 */
    mk_eval_result r4 = mk_evaluate("1+*2");
    check(r4.status == MK_ERR_PARSE && r4.error != NULL, "parse error mapping");
    printf("parse error: %s (pos %zu)\n", r4.error ? r4.error : "(null)", r4.error_pos);

    /* 错误路径：求值错误 → MK_ERR_EVAL */
    mk_eval_result r5 = mk_evaluate("1/0");
    check(r5.status == MK_ERR_EVAL && r5.error != NULL, "eval error mapping");
    printf("eval error: %s\n", r5.error ? r5.error : "(null)");

    /* 错误路径：空参数 → MK_ERR_INVALID_ARG */
    mk_eval_result r6 = mk_evaluate(NULL);
    check(r6.status == MK_ERR_INVALID_ARG, "invalid arg mapping");

    if (failures == 0) {
        printf("ALL OK\n");
        return 0;
    }
    return 1;
}
