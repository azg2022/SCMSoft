find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

# Windows 下以 GUI 子系统编译（不出现控制台黑窗）
set(MK_EXECUTABLE_TYPE "")
if(WIN32)
    set(MK_EXECUTABLE_TYPE WIN32)
endif()

add_executable(measurekit ${MK_EXECUTABLE_TYPE}
    src/main.cpp
    src/mainwindow.cpp
    src/formulapanel.cpp
    src/matrixtab.cpp
    src/calckeypad.cpp
)
target_include_directories(measurekit PRIVATE src)
target_link_libraries(measurekit PRIVATE Qt6::Widgets mk_kernel)

# 开发期种子公式目录；安装后自动回退到 exe 旁 / 安装目录（见 mainwindow.cpp）
target_compile_definitions(measurekit PRIVATE
    MK_SEED_DIR="${CMAKE_SOURCE_DIR}/formulas/seed")

install(TARGETS measurekit RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/formulas/seed DESTINATION share/measurekit)

if(WIN32)
    # 构建后自动收集 Qt DLL 到 exe 旁，便于本地拿到就能跑。
    # 优先使用 Qt6 自带的 windeployqt 目标（CI 环境 PATH 中不一定有）
    if(TARGET Qt6::windeployqt)
        add_custom_command(TARGET measurekit POST_BUILD
            COMMAND $<TARGET_FILE:Qt6::windeployqt> --no-translations $<TARGET_FILE:measurekit>)
    else()
        find_program(WINDEPLOYQT windeployqt)
        if(WINDEPLOYQT)
            add_custom_command(TARGET measurekit POST_BUILD
                COMMAND ${WINDEPLOYQT} --no-translations $<TARGET_FILE:measurekit>)
        else()
            message(WARNING "未找到 windeployqt，Windows 下需手动部署 Qt DLL")
        endif()
    endif()
endif()

if(UNIX AND NOT APPLE)
    install(FILES resources/measurekit.desktop DESTINATION share/applications)
    install(FILES resources/measurekit.svg DESTINATION share/icons/hicolor/scalable/apps)
endif()
