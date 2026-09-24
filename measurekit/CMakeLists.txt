cmake_minimum_required(VERSION 3.22)
# C：SDK 的纯 C 示例需要；CXX：内核与 SDK 主体
project(measurekit VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(MK_BUILD_APP "Build Qt6 desktop application" ON)
option(MK_BUILD_TESTS "Build unit tests" ON)
option(MK_BUILD_SDK "Build C ABI SDK layer" ON)

# enable_testing 必须在所有 add_subdirectory 之前，
# 否则 sdk 等子目录中注册的 add_test 不会进入 ctest
if(MK_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(kernel)

if(MK_BUILD_SDK)
    add_subdirectory(sdk)
endif()

if(MK_BUILD_APP)
    add_subdirectory(app)
endif()

if(MK_BUILD_TESTS)
    add_subdirectory(kernel/tests)
endif()

# ---------- 打包（CPack） ----------
# Linux 产出 .deb；Windows 由 CI 使用 NSIS/ZIP（见 .github/workflows）
set(CPACK_PACKAGE_VENDOR "MeasureKit")
set(CPACK_PACKAGE_CONTACT "azg2022")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "度量衡 MeasureKit · 智能科学计算器")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_PACKAGE_NAME "度量衡 MeasureKit")
    set(CPACK_NSIS_DISPLAY_NAME "MeasureKit")
else()
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "azg2022")
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6widgets6t64, libqt6gui6t64, libqt6core6t64, libsqlite3-0")
endif()
include(CPack)
