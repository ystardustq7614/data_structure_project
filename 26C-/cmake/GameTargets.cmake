include_guard(GLOBAL)
get_filename_component(GAME_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# 两个配置入口共享真实核心实现；不在静态库内嵌媒体或配置资源。
add_library(game_core STATIC
    ${GAME_SOURCE_ROOT}/core/attackplan.h
    ${GAME_SOURCE_ROOT}/core/attackplanloader.cpp
    ${GAME_SOURCE_ROOT}/core/attackplanloader.h
    ${GAME_SOURCE_ROOT}/core/attackscheduler.cpp
    ${GAME_SOURCE_ROOT}/core/attackscheduler.h
    ${GAME_SOURCE_ROOT}/core/gamemap.cpp
    ${GAME_SOURCE_ROOT}/core/gamemap.h
    ${GAME_SOURCE_ROOT}/core/gamesession.cpp
    ${GAME_SOURCE_ROOT}/core/gamesession.h
    ${GAME_SOURCE_ROOT}/core/enemy.cpp
    ${GAME_SOURCE_ROOT}/core/enemy.h
    ${GAME_SOURCE_ROOT}/core/tower.cpp
    ${GAME_SOURCE_ROOT}/core/tower.h
    ${GAME_SOURCE_ROOT}/core/bullet.cpp
    ${GAME_SOURCE_ROOT}/core/bullet.h
)
target_include_directories(game_core PUBLIC ${GAME_SOURCE_ROOT})
target_link_libraries(game_core PUBLIC Qt6::Core)
set(GAME_RENDER_SOURCES
    ${GAME_SOURCE_ROOT}/ui/gamerenderer.cpp
    ${GAME_SOURCE_ROOT}/ui/gamerenderer.h
    ${GAME_SOURCE_ROOT}/ui/gameuistate.h
    ${GAME_SOURCE_ROOT}/ui/buttonfeedback.cpp
    ${GAME_SOURCE_ROOT}/ui/buttonfeedback.h
)
set(GAME_UI_SOURCES
    ${GAME_RENDER_SOURCES}
    ${GAME_SOURCE_ROOT}/ui/mainwindow.cpp
    ${GAME_SOURCE_ROOT}/ui/mainwindow.h
    ${GAME_SOURCE_ROOT}/mainwindow.ui
    ${GAME_SOURCE_ROOT}/ui/gamewidget.cpp
    ${GAME_SOURCE_ROOT}/ui/gamewidget.h
    ${GAME_SOURCE_ROOT}/ui/startmenuwidget.cpp
    ${GAME_SOURCE_ROOT}/ui/startmenuwidget.h
)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${GAME_SOURCE_ROOT})
