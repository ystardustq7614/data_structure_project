cmake_minimum_required(VERSION 3.16)
project(tower_defense_regression LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

enable_testing()
configure_file(fixtures/legacy_spawn_trace.txt
               ${CMAKE_CURRENT_BINARY_DIR}/legacy_spawn_trace.txt COPYONLY)
add_executable(test_attack_scheduler
    test_attack_scheduler.cpp ../26C-/attackscheduler.cpp ../26C-/resources.qrc)
target_link_libraries(test_attack_scheduler PRIVATE Qt6::Widgets)
add_test(NAME attack_scheduler COMMAND test_attack_scheduler)
add_executable(test_enemy_product test_enemy_product.cpp ../26C-/enemy.cpp)
target_link_libraries(test_enemy_product PRIVATE Qt6::Widgets)
add_test(NAME enemy_product COMMAND test_enemy_product)

add_executable(test_gui_product
    test_gui_product.cpp
    ../26C-/mainwindow.cpp
    ../26C-/mainwindow.h
    ../26C-/mainwindow.ui
    ../26C-/gamewidget.cpp
    ../26C-/gamewidget.h
    ../26C-/attackscheduler.cpp
    ../26C-/attackscheduler.h
    ../26C-/startmenuwidget.cpp
    ../26C-/startmenuwidget.h
    ../26C-/enemy.cpp
    ../26C-/enemy.h
    ../26C-/tower.cpp
    ../26C-/tower.h
    ../26C-/bullet.cpp
    ../26C-/bullet.h
    ../26C-/resources.qrc
)
target_link_libraries(test_gui_product PRIVATE Qt6::Widgets Qt6::Multimedia)
add_test(NAME gui_product COMMAND test_gui_product)
