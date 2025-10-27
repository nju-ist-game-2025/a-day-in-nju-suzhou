cmake_minimum_required(VERSION 3.16)

project(game_final VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows specific settings
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

# 按文件夹组织源文件（全小写）
set(MAIN_SOURCES
    src/main.cpp
)

set(CORE_SOURCES
    src/core/gamewindow.cpp
    src/core/gamewindow.h
    src/core/gamecontroller.cpp
    src/core/gamecontroller.h
    src/core/gamemanager.cpp
    src/core/gamemanager.h
    src/core/audiomanager.cpp
    src/core/audiomanager.h
    src/core/resourcefactory.h
)

set(ENTITY_SOURCES
    src/entities/entity.cpp
    src/entities/entity.h
    src/entities/player.cpp
    src/entities/player.h
    src/entities/enemy.cpp
    src/entities/enemy.h
    src/entities/boss.cpp
    src/entities/boss.h
    src/entities/projectile.cpp
    src/entities/projectile.h
)

set(WORLD_SOURCES
    src/world/level.cpp
    src/world/level.h
    src/world/room.cpp
    src/world/room.h
    src/world/randomgenerator.cpp
    src/world/randomgenerator.h
)

set(ITEM_SOURCES
    src/items/item.cpp
    src/items/item.h
    src/items/chest.cpp
    src/items/chest.h
    src/items/statuseffect.cpp
    src/items/statuseffect.h
)

set(UI_SOURCES
    src/ui/hud.cpp
    src/ui/hud.h
    src/ui/mainmenu.cpp
    src/ui/mainmenu.h
    src/ui/codex.cpp
    src/ui/codex.h
    src/ui/gameview.cpp
    src/ui/gameview.h
)


# 合并所有源文件
set(PROJECT_SOURCES
    ${MAIN_SOURCES}
    ${CORE_SOURCES}
    ${ENTITY_SOURCES}
    ${WORLD_SOURCES}
    ${ITEM_SOURCES}
    ${UI_SOURCES}
    ${UI_FORMS}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(game_final
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/constants.h
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET game_final APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(game_final SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(game_final
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(game_final PRIVATE 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Multimedia
)

if(MINGW)
    target_link_options(game_final PRIVATE -Wl,--allow-multiple-definition)
endif()

# 包含头文件目录
target_include_directories(game_final PRIVATE
    src
    src/core
    src/entities
    src/world
    src/items
    src/ui
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.game_final)
endif()
set_target_properties(game_final PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS game_final
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(game_final)
endif()
