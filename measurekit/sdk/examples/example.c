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

    /* 牛顿求根：x^2-2=0 在 1 附近的根 = sqrt(2) */
    mk_eval_result r7 = mk_newton_root("x^2-2", "x", 1.0, 0.0);
    check(r7.status == MK_OK && fabs(r7.value - sqrt(2.0)) < 1e-10, "mk_newton_root");
    printf("root of x^2-2 ≈ %.15g\n", r7.value);

    /* 自适应辛普森积分：∫[0,1] x^2 dx = 1/3 */
    mk_eval_result r8 = mk_integrate("x^2", "x", 0.0, 1.0, 0.0);
    check(r8.status == MK_OK && fabs(r8.value - 1.0 / 3.0) < 1e-12, "mk_integrate");
    printf("∫[0,1] x^2 dx ≈ %.15g\n", r8.value);

    /* 常微分方程 y'=y, y(0)=1，步长 0.05 走 20 步 → y(1) ≈ e。
     * 输出每行为 "x\ty"，取最后一个制表符后的 y 值 */
    mk_string_result r9 = mk_solve_ode("y", "x", "y", 0.0, 1.0, 0.05, 20);
    double y_end = 0.0;
    const char* last_tab = (r9.status == MK_OK && r9.text) ? strrchr(r9.text, '\t') : NULL;
    if (last_tab)
        sscanf(last_tab + 1, "%lf", &y_end);
    check(r9.status == MK_OK && fabs(y_end - 2.718281828459045) < 1e-6, "mk_solve_ode");
    printf("y'=y, y(0)=1 → y(1) ≈ %.15g\n", y_end);
    mk_free_string(r9.text);

    /* 错误路径：积分区间内有奇点 → MK_ERR_NUMERIC */
    mk_eval_result r10 = mk_integrate("1/(x-0.5)", "x", 0.0, 1.0, 0.0);
    check(r10.status == MK_ERR_NUMERIC && r10.error != NULL, "numeric error mapping");
    printf("numeric error: %s\n", r10.error ? r10.error : "(null)");

    /* 错误路径：空参数 → MK_ERR_INVALID_ARG */
    mk_eval_result r6 = mk_evaluate(NULL);
    check(r6.status == MK_ERR_INVALID_ARG, "invalid arg mapping");

    if (failures == 0) {
        printf("ALL OK\n");
        return 0;
    }
    return 1;
}
