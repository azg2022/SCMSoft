find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

add_executable(measurekit
    src/main.cpp
    src/mainwindow.cpp
    src/formulapanel.cpp
    src/matrixtab.cpp
    src/calckeypad.cpp
)
target_include_directories(measurekit PRIVATE src)
target_link_libraries(measurekit PRIVATE Qt6::Widgets mk_kernel)

# 开发期种子公式目录；安装包改用 /usr/share/measurekit/seed（见 packaging）
target_compile_definitions(measurekit PRIVATE
    MK_SEED_DIR="${CMAKE_SOURCE_DIR}/formulas/seed")

install(TARGETS measurekit RUNTIME DESTINATION bin)
