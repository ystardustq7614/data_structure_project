# 产品入口与 tests 入口共享 game_core 和展示清单；避免测试复制另一套核心实现。
cmake_minimum_required(VERSION 3.16)

project(final VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)
include(cmake/GameTargets.cmake)
set(PROJECT_SOURCES main.cpp ${GAME_UI_SOURCES} resources.qrc)

qt_add_executable(final MANUAL_FINALIZATION ${PROJECT_SOURCES})
target_link_libraries(final PRIVATE
    game_core
    Qt6::Widgets
    Qt6::Multimedia
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(Qt6_VERSION VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.final)
endif()
set_target_properties(final PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
# 只复制示例；不覆盖玩家放在可执行文件旁的 attack_rhythm.json，新局由界面重新加载。
configure_file(attack_rhythm.example.json
               ${CMAKE_CURRENT_BINARY_DIR}/attack_rhythm.example.json COPYONLY)
install(TARGETS final
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES attack_rhythm.example.json DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_finalize_executable(final)
