// 公式引擎测试：仅在 SQLite 可用时编译（kernel 库同一探测条件）
#ifdef MK_HAVE_SQLITE

#include <filesystem>
#include <fstream>
#include <cmath>
#include <memory>
#include <unordered_map>

#include <gtest/gtest.h>

#include "mk/error.h"
#include "mk/formula.h"

namespace {

// 每个测试用独立的临时库文件，避免相互污染
std::string tempDbPath(const char* tag) {
    auto path = std::filesystem::temp_directory_path() /
                ("mk_test_formula_" + std::string(tag) + ".db");
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return path.string();
}

std::unique_ptr<mk::FormulaEngine> makeSeeded(const char* tag) {
    auto engine = std::make_unique<mk::FormulaEngine>(tempDbPath(tag));
    engine->seedFromJsonDir(MK_SEED_DIR);
    return engine;
}

TEST(Formula, SeedCountsAndIdempotence) {
    auto engine = makeSeeded("counts");
    const int kCategories = 5;
    int total = 0;
    for (const char* cat : {"general", "probability", "statistics", "linalg", "calculus"}) {
        auto list = engine->listByCategory(cat);
        EXPECT_GE(static_cast<int>(list.size()), 15) << "category " << cat;
        total += static_cast<int>(list.size());
    }
    EXPECT_GE(total, 75);

    // 幂等：再次加载数量不变
    engine->seedFromJsonDir(MK_SEED_DIR);
    int again = 0;
    for (const char* cat : {"general", "probability", "statistics", "linalg", "calculus"})
        again += static_cast<int>(engine->listByCategory(cat).size());
    EXPECT_EQ(total, again);
}

TEST(Formula, AllSeedExprsParseAndEvalWithDefaults) {
    auto engine = makeSeeded("quality");
    for (const char* cat : {"general", "probability", "statistics", "linalg", "calculus"}) {
        for (const auto& f : engine->listByCategory(cat)) {
            SCOPED_TRACE(f.name);
            EXPECT_NO_THROW({
                double v = engine->evaluate(f, {});
                EXPECT_TRUE(std::isfinite(v));
            });
        }
    }
}

TEST(Formula, QuadraticPositiveRoot) {
    auto engine = makeSeeded("quad");
    auto list = engine->search("一元二次方程求根公式（正根）");
    ASSERT_EQ(list.size(), 1u);
    std::unordered_map<std::string, double> bindings = {{"a", 1}, {"b", -3}, {"c", 2}};
    EXPECT_NEAR(engine->evaluate(list.front(), bindings), 2.0, 1e-12);
}

TEST(Formula, SearchAndList) {
    auto engine = makeSeeded("search");
    auto hits = engine->search("二次");
    EXPECT_GE(hits.size(), 3u);
    EXPECT_TRUE(engine->search("不存在的公式名xyz").empty());
    EXPECT_EQ(engine->listByCategory("general").size(), 18u);
}

TEST(Formula, AddCustomValidAndInvalid) {
    auto engine = makeSeeded("custom");
    const std::string params = "[{\"symbol\":\"r\",\"label\":\"半径\",\"unit\":\"\",\"domain\":\"r>0\",\"default\":1}]";
    long long id = engine->addCustom("测试公式", "A=\\pi r^2", "pi*r^2", params, "general");
    EXPECT_GT(id, 0);
    auto f = engine->get(id);
    EXPECT_FALSE(f.builtin);
    EXPECT_NEAR(engine->evaluate(f, {{"r", 2}}), 4 * 3.141592653589793, 1e-9);

    // 无效表达式被拒
    EXPECT_THROW(engine->addCustom("坏公式", "", "2+*3", params, "general"), mk::MkError);
    // 无效参数表被拒
    EXPECT_THROW(engine->addCustom("坏参数", "", "r", "not json", "general"), mk::MkError);
}

TEST(Formula, RemoveCustomGuardsBuiltin) {
    auto engine = makeSeeded("remove");
    const std::string params = "[{\"symbol\":\"x\",\"label\":\"x\",\"unit\":\"\",\"domain\":\"\",\"default\":1}]";
    long long id = engine->addCustom("待删除", "", "x+1", params, "general");

    // 内置公式不可删除
    auto builtin = engine->search("勾股定理（斜边）");
    ASSERT_EQ(builtin.size(), 1u);
    EXPECT_FALSE(engine->removeCustom(builtin.front().id));
    EXPECT_NO_THROW(engine->get(builtin.front().id));

    // 自定义公式可删除
    EXPECT_TRUE(engine->removeCustom(id));
    EXPECT_THROW(engine->get(id), mk::MkError);
}

} // namespace

#endif // MK_HAVE_SQLITE
