add_library(mk_kernel STATIC
    src/ast.cpp
    src/tokenizer.cpp
    src/parser.cpp
    src/evaluator.cpp
    src/printer.cpp
    src/simplify.cpp
    src/differentiate.cpp
)
target_include_directories(mk_kernel PUBLIC include)

# 内核以静态库形式被链接进共享库 measurekit_sdk：需要位置无关代码；
# 隐藏可见性使内核符号不会经共享库泄漏进 ABI（SDK 只导出 MK_API 标注的函数）
set_target_properties(mk_kernel PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# M2：找到 Eigen 时编译线性代数模块并定义 MK_HAVE_EIGEN；
# SQLite 找到时仅预留编译定义与链接，对应源文件将在 M3 里程碑加入。
find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    target_compile_definitions(mk_kernel PUBLIC MK_HAVE_EIGEN)
    target_link_libraries(mk_kernel PUBLIC Eigen3::Eigen)
    target_sources(mk_kernel PRIVATE src/linalg.cpp)
endif()

find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    target_compile_definitions(mk_kernel PUBLIC MK_HAVE_SQLITE)
    target_link_libraries(mk_kernel PUBLIC SQLite::SQLite3)
    target_sources(mk_kernel PRIVATE src/formula.cpp)
endif()

add_executable(mk_cli src/cli.cpp)
target_link_libraries(mk_cli PRIVATE mk_kernel)
