add_library(measurekit_sdk SHARED src/api.cpp)
target_include_directories(measurekit_sdk PUBLIC include)
target_link_libraries(measurekit_sdk PRIVATE mk_kernel)

# 符号导出控制：内部符号不进 ABI，仅头文件中 MK_API 标注的 5 个函数导出
set_target_properties(measurekit_sdk PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    VERSION 0.1.0
    SOVERSION 0
)

# 纯 C 示例：验证内核能力经 C ABI 对外开放
add_executable(mk_sdk_example examples/example.c)
target_link_libraries(mk_sdk_example PRIVATE measurekit_sdk)

if(MK_BUILD_TESTS)
    add_test(NAME sdk_example COMMAND mk_sdk_example)
endif()
