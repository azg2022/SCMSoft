#include "mk/formula.h"

#ifdef MK_HAVE_SQLITE

#include <sqlite3.h>

#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>

#include "mk/error.h"
#include "mk/evaluator.h"
#include "mk/parser.h"

namespace mk {

namespace {

// ---------- 极简 JSON 解析器 ----------
// 仅支持种子数据与 params 字段所需的子集：object / array / string / number / true / false / null。
struct Json {
    enum class Kind { Null, Bool, Number, String, Array, Object } kind = Kind::Null;
    double num = 0.0;
    bool boolean = false;
    std::string str;
    std::vector<Json> arr;
    std::vector<std::pair<std::string, Json>> obj;

    const Json* find(const std::string& key) const {
        if (kind != Kind::Object) return nullptr;
        for (const auto& kv : obj)
            if (kv.first == key) return &kv.second;
        return nullptr;
    }
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text) : text_(text) {}

    Json parse() {
        skipWs();
        Json v = parseValue();
        skipWs();
        if (pos_ != text_.size())
            throw MkError("JSON 解析失败：末尾有多余内容");
        return v;
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;

    void skipWs() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])))
            ++pos_;
    }

    char peek() {
        if (pos_ >= text_.size()) throw MkError("JSON 解析失败：意外结束");
        return text_[pos_];
    }

    bool consume(char c) {
        skipWs();
        if (pos_ < text_.size() && text_[pos_] == c) { ++pos_; return true; }
        return false;
    }

    void expect(char c) {
        if (!consume(c)) throw MkError(std::string("JSON 解析失败：应为 '") + c + "'");
    }

    Json parseValue() {
        skipWs();
        char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return parseString();
        if (c == 't') { literal("true"); Json v; v.kind = Json::Kind::Bool; v.boolean = true; return v; }
        if (c == 'f') { literal("false"); Json v; v.kind = Json::Kind::Bool; return v; }
        if (c == 'n') { literal("null"); return Json{}; }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber();
        throw MkError(std::string("JSON 解析失败：意外的字符 '") + c + "'");
    }

    void literal(const char* word) {
        std::size_t len = std::strlen(word);
        if (text_.compare(pos_, len, word) != 0)
            throw MkError("JSON 解析失败：非法字面量");
        pos_ += len;
    }

    Json parseNumber() {
        std::size_t start = pos_;
        while (pos_ < text_.size() &&
               (std::isdigit(static_cast<unsigned char>(text_[pos_])) ||
                text_[pos_] == '-' || text_[pos_] == '+' || text_[pos_] == '.' ||
                text_[pos_] == 'e' || text_[pos_] == 'E'))
            ++pos_;
        Json v;
        v.kind = Json::Kind::Number;
        v.num = std::stod(text_.substr(start, pos_ - start));
        return v;
    }

    Json parseString() {
        expect('"');
        std::string s;
        while (true) {
            if (pos_ >= text_.size()) throw MkError("JSON 解析失败：字符串未闭合");
            char c = text_[pos_++];
            if (c == '"') break;
            if (c == '\\') {
                if (pos_ >= text_.size()) throw MkError("JSON 解析失败：转义符意外结束");
                char e = text_[pos_++];
                switch (e) {
                case '"': s += '"'; break;
                case '\\': s += '\\'; break;
                case '/': s += '/'; break;
                case 'b': s += '\b'; break;
                case 'f': s += '\f'; break;
                case 'n': s += '\n'; break;
                case 'r': s += '\r'; break;
                case 't': s += '\t'; break;
                case 'u':
                    if (pos_ + 4 > text_.size()) throw MkError("JSON 解析失败：非法 \\u 转义");
                    pos_ += 4; // 种子数据不含非 ASCII 转义，直接跳过
                    s += '?';
                    break;
                default: throw MkError("JSON 解析失败：非法转义符");
                }
            } else {
                s += c;
            }
        }
        Json v;
        v.kind = Json::Kind::String;
        v.str = std::move(s);
        return v;
    }

    Json parseArray() {
        expect('[');
        Json v;
        v.kind = Json::Kind::Array;
        skipWs();
        if (consume(']')) return v;
        while (true) {
            v.arr.push_back(parseValue());
            if (consume(',')) continue;
            expect(']');
            return v;
        }
    }

    Json parseObject() {
        expect('{');
        Json v;
        v.kind = Json::Kind::Object;
        skipWs();
        if (consume('}')) return v;
        while (true) {
            skipWs();
            std::string key = parseString().str;
            expect(':');
            v.obj.emplace_back(std::move(key), parseValue());
            if (consume(',')) continue;
            expect('}');
            return v;
        }
    }
};

std::string jsonStr(const Json& j, const std::string& def = "") {
    return j.kind == Json::Kind::String ? j.str : def;
}

double jsonNum(const Json& j, double def = 0.0) {
    return j.kind == Json::Kind::Number ? j.num : def;
}

std::vector<FormulaParam> paramsFromJson(const Json& root) {
    std::vector<FormulaParam> params;
    if (root.kind != Json::Kind::Array)
        throw MkError("参数表必须是 JSON 数组");
    for (const Json& p : root.arr) {
        FormulaParam fp;
        fp.symbol = jsonStr(*p.find("symbol"));
        fp.label = jsonStr(*p.find("label"));
        fp.unit = jsonStr(*p.find("unit"));
        fp.domain = jsonStr(*p.find("domain"));
        fp.defaultValue = jsonNum(*p.find("default"), 0.0);
        if (fp.symbol.empty()) throw MkError("参数缺少 symbol 字段");
        params.push_back(std::move(fp));
    }
    return params;
}

std::vector<FormulaParam> parseParams(const std::string& paramsJson) {
    return paramsFromJson(JsonParser(paramsJson).parse());
}

std::string paramsToJson(const std::vector<FormulaParam>& params) {
    std::ostringstream os;
    os << '[';
    bool first = true;
    for (const auto& p : params) {
        if (!first) os << ',';
        first = false;
        os << "{\"symbol\":\"" << p.symbol << "\",\"label\":\"" << p.label
           << "\",\"unit\":\"" << p.unit << "\",\"domain\":\"" << p.domain
           << "\",\"default\":" << p.defaultValue << '}';
    }
    os << ']';
    return os.str();
}

// ---------- SQLite 辅助 ----------

const char* kSchema = R"SQL(
CREATE TABLE IF NOT EXISTS formulas (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    latex TEXT NOT NULL,
    expr TEXT NOT NULL,
    params TEXT NOT NULL,
    category TEXT NOT NULL,
    builtin INTEGER NOT NULL DEFAULT 1
);
CREATE INDEX IF NOT EXISTS idx_formulas_category ON formulas(category);
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
        throw MkError("无法打开公式数据库：" + msg);
    }
    return db;
}

Formula rowToFormula(sqlite3_stmt* stmt) {
    Formula f;
    f.id = sqlite3_column_int64(stmt, 0);
    f.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    f.latex = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    f.expr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    const auto* paramsJson = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    f.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    f.builtin = sqlite3_column_int(stmt, 6) != 0;
    if (paramsJson) f.params = parseParams(paramsJson);
    return f;
}

} // namespace

FormulaEngine::FormulaEngine(const std::string& dbPath)
    : db_(openDb(dbPath)) {
    execChecked(static_cast<sqlite3*>(db_), kSchema);
}

FormulaEngine::~FormulaEngine() {
    if (db_) sqlite3_close(static_cast<sqlite3*>(db_));
}

void FormulaEngine::seedFromJsonDir(const std::string& dir) {
    // 目录内 *.json 按文件名排序，保证种子 id 稳定
    std::vector<std::string> files;
    std::ifstream listing;
    (void)listing;
    // 种子目录只包含 5 个已知文件，直接枚举
    static const char* kNames[] = {"general.json", "probability.json", "statistics.json",
                                   "linalg.json", "calculus.json"};
    for (const char* name : kNames) {
        std::string path = dir;
        if (!path.empty() && path.back() != '/') path += '/';
        path += name;
        std::ifstream in(path);
        if (!in) throw MkError(std::string("无法打开种子文件：") + path);
        files.push_back(std::move(path));
    }
    for (const auto& path : files) {
        std::ifstream in(path);
        std::ostringstream ss;
        ss << in.rdbuf();
        Json root = JsonParser(ss.str()).parse();
        if (root.kind != Json::Kind::Array)
            throw MkError("种子文件顶层必须是数组：" + path);
        for (const Json& item : root.arr) {
            Formula f;
            f.name = jsonStr(*item.find("name"));
            f.latex = jsonStr(*item.find("latex"));
            f.expr = jsonStr(*item.find("expr"));
            f.category = jsonStr(*item.find("category"));
            f.builtin = true;
            f.params = paramsFromJson(*item.find("params"));

            // 入库前质量闸门：expr 必须能被内核解析
            parse(f.expr);

            sqlite3* db = static_cast<sqlite3*>(db_);
            // 幂等：name+category 判重
            sqlite3_stmt* q = nullptr;
            sqlite3_prepare_v2(db, "SELECT id FROM formulas WHERE name=? AND category=?", -1, &q, nullptr);
            sqlite3_bind_text(q, 1, f.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(q, 2, f.category.c_str(), -1, SQLITE_TRANSIENT);
            const bool exists = sqlite3_step(q) == SQLITE_ROW;
            sqlite3_finalize(q);
            if (exists) continue;

            const std::string paramsJson = paramsToJson(f.params);
            sqlite3_stmt* ins = nullptr;
            sqlite3_prepare_v2(db,
                               "INSERT INTO formulas(name,latex,expr,params,category,builtin) "
                               "VALUES(?,?,?,?,?,1)", -1, &ins, nullptr);
            sqlite3_bind_text(ins, 1, f.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(ins, 2, f.latex.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(ins, 3, f.expr.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(ins, 4, paramsJson.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(ins, 5, f.category.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(ins) != SQLITE_DONE) {
                std::string msg = sqlite3_errmsg(db);
                sqlite3_finalize(ins);
                throw MkError("公式入库失败：" + msg);
            }
            sqlite3_finalize(ins);
        }
    }
}

std::vector<Formula> FormulaEngine::listByCategory(const std::string& category) {
    sqlite3* db = static_cast<sqlite3*>(db_);
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db,
                       "SELECT id,name,latex,expr,params,category,builtin FROM formulas "
                       "WHERE category=? ORDER BY id", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<Formula> out;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        out.push_back(rowToFormula(stmt));
    sqlite3_finalize(stmt);
    return out;
}

std::vector<Formula> FormulaEngine::search(const std::string& keyword) {
    sqlite3* db = static_cast<sqlite3*>(db_);
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db,
                       "SELECT id,name,latex,expr,params,category,builtin FROM formulas "
                       "WHERE name LIKE ? ESCAPE '\\' ORDER BY category, id", -1, &stmt, nullptr);
    // 转义 LIKE 通配符
    std::string like = "%";
    for (char c : keyword) {
        if (c == '%' || c == '_' || c == '\\') like += '\\';
        like += c;
    }
    like += '%';
    sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<Formula> out;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        out.push_back(rowToFormula(stmt));
    sqlite3_finalize(stmt);
    return out;
}

Formula FormulaEngine::get(long long id) {
    sqlite3* db = static_cast<sqlite3*>(db_);
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db,
                       "SELECT id,name,latex,expr,params,category,builtin FROM formulas WHERE id=?", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    Formula f;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        f = rowToFormula(stmt);
    else {
        sqlite3_finalize(stmt);
        throw MkError("公式不存在（id=" + std::to_string(id) + "）");
    }
    sqlite3_finalize(stmt);
    return f;
}

double FormulaEngine::evaluate(const Formula& f,
                               const std::unordered_map<std::string, double>& bindings) {
    Env env;
    for (const auto& p : f.params) {
        auto it = bindings.find(p.symbol);
        if (it != bindings.end()) {
            env[p.symbol] = it->second;
        } else {
            env[p.symbol] = p.defaultValue;
        }
    }
    NodePtr ast = parse(f.expr);
    return mk::evaluate(*ast, env);
}

long long FormulaEngine::addCustom(const std::string& name, const std::string& latex,
                                   const std::string& expr, const std::string& paramsJson,
                                   const std::string& category) {
    parse(expr); // 静态校验
    parseParams(paramsJson); // 参数表结构校验

    sqlite3* db = static_cast<sqlite3*>(db_);
    sqlite3_stmt* ins = nullptr;
    sqlite3_prepare_v2(db,
                       "INSERT INTO formulas(name,latex,expr,params,category,builtin) "
                       "VALUES(?,?,?,?,?,0)", -1, &ins, nullptr);
    sqlite3_bind_text(ins, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(ins, 2, latex.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(ins, 3, expr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(ins, 4, paramsJson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(ins, 5, category.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(ins) != SQLITE_DONE) {
        std::string msg = sqlite3_errmsg(db);
        sqlite3_finalize(ins);
        throw MkError("公式入库失败：" + msg);
    }
    const long long id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(ins);
    return id;
}

bool FormulaEngine::removeCustom(long long id) {
    sqlite3* db = static_cast<sqlite3*>(db_);
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, "DELETE FROM formulas WHERE id=? AND builtin=0", -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    const bool removed = sqlite3_changes(db) > 0;
    sqlite3_finalize(stmt);
    return removed;
}

} // namespace mk

#endif // MK_HAVE_SQLITE
