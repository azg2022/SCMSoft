#include "mk/history.h"

#ifdef MK_HAVE_SQLITE

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "mk/error.h"

namespace {

// 每个用例独立的临时数据库文件
class HistoryStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = std::string("/tmp/mk_history_test_") +
                std::to_string(::testing::UnitTest::GetInstance()->random_seed()) + ".db";
        std::remove(path_.c_str());
    }
    void TearDown() override { std::remove(path_.c_str()); }

    std::string path_;
};

} // namespace

TEST_F(HistoryStoreTest, AddAndListNewestFirst) {
    mk::HistoryStore store(path_);
    store.add("2+3*4", "14");
    store.add("sqrt(2)", "1.41421356237");
    store.add("sin(pi/2)", "1");

    auto entries = store.list();
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].expr, "sin(pi/2)"); // 最新在前
    EXPECT_EQ(entries[0].result, "1");
    EXPECT_EQ(entries[2].expr, "2+3*4");
    EXPECT_GT(entries[0].id, entries[2].id);
}

TEST_F(HistoryStoreTest, ListRespectsLimit) {
    mk::HistoryStore store(path_);
    for (int i = 0; i < 10; ++i)
        store.add("e" + std::to_string(i), std::to_string(i));

    auto entries = store.list(4);
    ASSERT_EQ(entries.size(), 4u);
    EXPECT_EQ(entries[0].expr, "e9");
}

TEST_F(HistoryStoreTest, ClearEmptiesStore) {
    mk::HistoryStore store(path_);
    store.add("1+1", "2");
    store.clear();
    EXPECT_TRUE(store.list().empty());
}

TEST_F(HistoryStoreTest, PersistsAcrossInstances) {
    {
        mk::HistoryStore store(path_);
        store.add("x=5", "5");
    }
    mk::HistoryStore reopened(path_);
    auto entries = reopened.list();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].expr, "x=5");
}

TEST_F(HistoryStoreTest, CsvExportWithEscaping) {
    mk::HistoryStore store(path_);
    store.add("2+3*4", "14");
    store.add("a,b", "say \"hi\""); // 含逗号与引号，需转义

    const std::string csv = store.toCsv();
    std::istringstream lines(csv);
    std::string line;
    std::getline(lines, line);
    EXPECT_EQ(line, "id,expr,result");

    std::getline(lines, line);
    EXPECT_EQ(line, "2,\"a,b\",\"say \"\"hi\"\"\""); // 最新在前
    std::getline(lines, line);
    EXPECT_EQ(line, "1,2+3*4,14");
    EXPECT_TRUE(std::getline(lines, line).fail()); // 已无更多行
}

TEST_F(HistoryStoreTest, EmptyCsvHasHeaderOnly) {
    mk::HistoryStore store(path_);
    EXPECT_EQ(store.toCsv(), "id,expr,result\n");
}

TEST_F(HistoryStoreTest, InvalidLimitThrows) {
    mk::HistoryStore store(path_);
    EXPECT_THROW(store.list(0), mk::MkError);
}

TEST_F(HistoryStoreTest, OpenFailureThrows) {
    EXPECT_THROW(mk::HistoryStore("/proc/1/definitely-not-writable/x.db"), mk::MkError);
}

#endif // MK_HAVE_SQLITE
