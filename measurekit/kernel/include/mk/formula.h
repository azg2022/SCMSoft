#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace mk {

struct FormulaParam {
    std::string symbol;  // 参数符号，与 expr 中的变量名对应
    std::string label;   // 中文说明
    std::string unit;    // 单位，可为空
    std::string domain;  // 定义域描述（展示用），可为空
    double defaultValue = 0.0;
};

struct Formula {
    long long id = 0;
    std::string name;
    std::string latex;
    std::string expr;
    std::string category;
    bool builtin = true;
    std::vector<FormulaParam> params;
};

// 公式引擎：SQLite 持久化 + 参数绑定求值。
// 仅在 MK_HAVE_SQLITE 下编译（SQLite3 未安装时禁用）。
class FormulaEngine {
public:
    explicit FormulaEngine(const std::string& dbPath);
    ~FormulaEngine();

    FormulaEngine(const FormulaEngine&) = delete;
    FormulaEngine& operator=(const FormulaEngine&) = delete;

    // 从目录加载所有 *.json 种子文件，按 name+category 幂等判重
    void seedFromJsonDir(const std::string& dir);

    std::vector<Formula> listByCategory(const std::string& category);
    std::vector<Formula> search(const std::string& keyword);
    Formula get(long long id);

    // 参数未绑定时回退到默认值；既未绑定又无默认值的参数抛 MkError
    double evaluate(const Formula& f, const std::unordered_map<std::string, double>& bindings);

    // 入库前用内核 Parser 静态校验 expr；返回新公式 id
    long long addCustom(const std::string& name, const std::string& latex,
                        const std::string& expr, const std::string& paramsJson,
                        const std::string& category);

    // 仅允许删除 builtin=0 的公式；删除成功返回 true
    bool removeCustom(long long id);

private:
    void* db_ = nullptr; // sqlite3*
};

} // namespace mk
