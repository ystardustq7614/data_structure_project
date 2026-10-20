# 独立测试入口直接链接当前产品实现；历史独立模型 test_enemy_aura 不属于这五组回归。
cmake_minimum_required(VERSION 3.16)
project(tower_defense_regression LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

enable_testing()
include(../26C-/cmake/GameTargets.cmake)
configure_file(fixtures/legacy_spawn_trace.txt
               ${CMAKE_CURRENT_BINARY_DIR}/legacy_spawn_trace.txt COPYONLY)
add_executable(test_attack_scheduler
    test_attack_scheduler.cpp config_resources.qrc)
target_link_libraries(test_attack_scheduler PRIVATE game_core)
add_test(NAME attack_scheduler COMMAND test_attack_scheduler)
add_executable(test_enemy_product test_enemy_product.cpp)
target_link_libraries(test_enemy_product PRIVATE game_core)
add_test(NAME enemy_product COMMAND test_enemy_product)

add_executable(test_session_product test_session_product.cpp config_resources.qrc)
target_link_libraries(test_session_product PRIVATE game_core)
add_test(NAME session_product COMMAND test_session_product)

# 备用绘制目标不带图片资源，GUI 目标带完整真实资源。
add_executable(test_entity_rendering test_entity_rendering.cpp ${GAME_RENDER_SOURCES})
target_link_libraries(test_entity_rendering PRIVATE game_core Qt6::Gui)
add_test(NAME entity_rendering COMMAND test_entity_rendering)
add_executable(test_gui_product test_gui_product.cpp ${GAME_UI_SOURCES}
               ${GAME_SOURCE_ROOT}/resources.qrc)
target_link_libraries(test_gui_product PRIVATE game_core Qt6::Widgets Qt6::Multimedia)
add_test(NAME gui_product COMMAND test_gui_product)
# 离屏平台验证 Qt 绘制与事件；真实桌面字体、设备输入和音频听感仍需人工验收。
set_tests_properties(entity_rendering gui_product PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
