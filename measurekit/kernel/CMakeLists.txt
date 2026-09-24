add_library(mk_kernel STATIC
    src/ast.cpp
    src/tokenizer.cpp
    src/parser.cpp
    src/evaluator.cpp
    src/printer.cpp
    src/simplify.cpp
    src/differentiate.cpp
    src/numerics.cpp
)
target_include_directories(mk_kernel PUBLIC include)

# 内核以静态库形式被链接进共享库 measurekit_sdk：需要位置无关代码；
# 隐藏可见性使内核符号不会经共享库泄漏进 ABI（SDK 只导出 MK_API 标注的函数）
set_target_properties(mk_kernel PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Eigen：优先用系统安装；找不到时（如 Windows CI）自动下载 3.4.0 源码构建
option(MK_FETCH_DEPS "找不到 Eigen/SQLite 时自动下载源码构建" ON)
find_package(Eigen3 QUIET)
if(NOT Eigen3_FOUND AND MK_FETCH_DEPS)
    message(STATUS "MeasureKit: 系统未找到 Eigen3，正在下载源码构建 (FetchContent)")
    include(FetchContent)
    FetchContent_Declare(eigen
        URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.bz2
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(eigen)
    if(TARGET Eigen3::Eigen)
        set(Eigen3_FOUND TRUE)
    endif()
endif()
if(Eigen3_FOUND)
    target_compile_definitions(mk_kernel PUBLIC MK_HAVE_EIGEN)
    target_link_libraries(mk_kernel PUBLIC Eigen3::Eigen)
    target_sources(mk_kernel PRIVATE src/linalg.cpp)
else()
    message(STATUS "MeasureKit: Eigen3 NOT found, linear algebra disabled")
endif()

# SQLite：优先用系统安装；找不到时下载官方 amalgamation 构建
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND AND MK_FETCH_DEPS)
    message(STATUS "MeasureKit: 系统未找到 SQLite3，正在下载 amalgamation 构建 (FetchContent)")
    include(FetchContent)
    FetchContent_Declare(sqlite3
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(sqlite3)
    add_library(mk_sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
    target_include_directories(mk_sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
    add_library(SQLite::SQLite3 ALIAS mk_sqlite3)
    set(SQLite3_FOUND TRUE)
endif()
if(SQLite3_FOUND)
    target_compile_definitions(mk_kernel PUBLIC MK_HAVE_SQLITE)
    target_link_libraries(mk_kernel PUBLIC SQLite::SQLite3)
    target_sources(mk_kernel PRIVATE src/formula.cpp)
else()
    message(STATUS "MeasureKit: SQLite3 NOT found, formula engine disabled")
endif()

add_executable(mk_cli src/cli.cpp)
target_link_libraries(mk_cli PRIVATE mk_kernel)
