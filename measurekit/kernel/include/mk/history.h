#pragma once

#include <string>
#include <vector>

namespace mk {

// 历史记录条目：expr 为表达式原文，result 为结果文本
struct HistoryEntry {
    long long id = 0;
    std::string expr;
    std::string result;
};

// 历史记录存储：SQLite 持久化（数据层共用能力，桌面/移动端复用）。
// 仅在 MK_HAVE_SQLITE 下编译（SQLite3 未安装时禁用）。
class HistoryStore {
public:
    explicit HistoryStore(const std::string& dbPath);
    ~HistoryStore();

    HistoryStore(const HistoryStore&) = delete;
    HistoryStore& operator=(const HistoryStore&) = delete;

    void add(const std::string& expr, const std::string& result);

    // 最新在前，最多 limit 条
    std::vector<HistoryEntry> list(int limit = 500) const;

    void clear();

    // 导出 CSV（UTF-8，含表头 id,expr,result，字段按 CSV 规则加引号转义），最新在前
    std::string toCsv() const;

private:
    void* db_ = nullptr; // sqlite3*
};

} // namespace mk
