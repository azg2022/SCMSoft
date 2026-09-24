find_package(GTest REQUIRED)

add_executable(mk_tests
    test_tokenizer.cpp
    test_parser.cpp
    test_evaluator.cpp
    test_printer.cpp
    test_differentiate.cpp
    test_simplify.cpp
    test_numerics.cpp
    test_evalsteps.cpp
)
target_link_libraries(mk_tests PRIVATE mk_kernel GTest::gtest_main)

# linalg 测试仅在 Eigen 可用时编译（与 kernel 库同一探测条件）
find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    target_sources(mk_tests PRIVATE test_linalg.cpp)
    target_compile_definitions(mk_tests PRIVATE MK_HAVE_EIGEN)
endif()

# 公式引擎测试仅在 SQLite 可用时编译（与 kernel 库同一探测条件）
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    target_sources(mk_tests PRIVATE test_formula.cpp)
    target_compile_definitions(mk_tests PRIVATE
        MK_HAVE_SQLITE
        MK_SEED_DIR="${CMAKE_SOURCE_DIR}/formulas/seed")
endif()

include(GoogleTest)
gtest_discover_tests(mk_tests)
