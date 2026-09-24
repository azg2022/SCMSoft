#include "mk/history.h"

#ifdef MK_HAVE_SQLITE

#include <sqlite3.h>

#include <sstream>

#include "mk/error.h"

namespace mk {

namespace {

const char* kSchema = R"SQL(
CREATE TABLE IF NOT EXISTS history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    expr TEXT NOT NULL,
    result TEXT NOT NULL
);
)SQL";

void execChecked(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "未知 SQLite 错误";
        sqlite3_free(err);
        throw MkError("数据库错误：" + msg);
    }
}

sqlite3* openDb(const std::string& path) {
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        std::string msg = db ? sqlite3_errmsg(db) : "无法分配句柄";
        if (db) sqlite3_close(db);
        throw MkError("无法打开历史记录数据库：" + msg);
    }
    return db;
}

// CSV 字段转义：含逗号/引号/换行时加引号并把内部引号翻倍
std::string csvField(const std::string& s) {
    if (s.find_first_of(",\"\n") == std::string::npos)
        return s;
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

} // namespace

HistoryStore::HistoryStore(const std::string& dbPath)
    : db_(openDb(dbPath)) {
    execChecked(static_cast<sqlite3*>(db_), kSchema);
}

HistoryStore::~HistoryStore() {
    if (db_) sqlite3_close(static_cast<sqlite3*>(db_));
}

void HistoryStore::add(const std::string& expr, const std::string& result) {
    const char* sql = "INSERT INTO history (expr, result) VALUES (?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = static_cast<sqlite3*>(db_);
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw MkError(std::string("数据库错误：") + sqlite3_errmsg(db));
    sqlite3_bind_text(stmt, 1, expr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, result.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::string msg = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw MkError("数据库错误：" + msg);
    }
    sqlite3_finalize(stmt);
}

std::vector<HistoryEntry> HistoryStore::list(int limit) const {
    if (limit < 1)
        throw MkError("查询条数必须 ≥ 1");

    const char* sql = "SELECT id, expr, result FROM history ORDER BY id DESC LIMIT ?";
    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = static_cast<sqlite3*>(db_);
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw MkError(std::string("数据库错误：") + sqlite3_errmsg(db));
    sqlite3_bind_int(stmt, 1, limit);

    std::vector<HistoryEntry> out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryEntry e;
        e.id = sqlite3_column_int64(stmt, 0);
        const auto* expr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const auto* result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        e.expr = expr ? expr : "";
        e.result = result ? result : "";
        out.push_back(std::move(e));
    }
    sqlite3_finalize(stmt);
    return out;
}

void HistoryStore::clear() {
    execChecked(static_cast<sqlite3*>(db_), "DELETE FROM history");
}

std::string HistoryStore::toCsv() const {
    std::ostringstream os;
    os << "id,expr,result\n";
    for (const auto& e : list(100000))
        os << e.id << ',' << csvField(e.expr) << ',' << csvField(e.result) << '\n';
    return os.str();
}

} // namespace mk

#endif // MK_HAVE_SQLITE
