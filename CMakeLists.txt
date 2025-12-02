cmake_minimum_required(VERSION 3.16)

project(game_final VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 修复Qt 6.8.3与MinGW的noexcept兼容性问题
if (MINGW)
    add_compile_definitions(QT_FORCE_ASSERTS)
    # 禁用noexcept静态断言错误
    add_compile_options(-Wno-error -fpermissive)
endif ()

# Option: build as Windows GUI (no console). Default OFF for maximum compatibility.
option(BUILD_WIN32_EXECUTABLE "Build as Windows GUI executable (no console)" OFF)

# Windows specific settings
if (WIN32 AND BUILD_WIN32_EXECUTABLE)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif ()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

# 按文件夹组织源文件（全小写）
set(MAIN_SOURCES
        src/main.cpp
)

set(CORE_SOURCES
        src/core/gamewindow.cpp
        src/core/gamewindow.h
        src/core/audiomanager.cpp
        src/core/audiomanager.h
        src/core/resourcefactory.h
        src/core/configmanager.cpp
        src/core/configmanager.h
        src/core/configvalidator.cpp
        src/core/configvalidator.h
        src/core/logging.cpp
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
        src/entities/level_1/nightmareboss.cpp
        src/entities/level_1/nightmareboss.h
        src/entities/level_2/washmachineboss.cpp
        src/entities/level_2/washmachineboss.h
        src/entities/level_3/teacherboss.cpp
        src/entities/level_3/teacherboss.h
        src/entities/level_3/invigilator.cpp
        src/entities/level_3/invigilator.h
        src/entities/level_3/chalkbeam.cpp
        src/entities/level_3/chalkbeam.h
        src/entities/level_3/exampaper.cpp
        src/entities/level_3/exampaper.h
        src/entities/level_3/mletrap.cpp
        src/entities/level_3/mletrap.h
        src/entities/level_1/clockenemy.cpp
        src/entities/level_1/clockenemy.h
        src/entities/level_1/clockboom.cpp
        src/entities/level_1/clockboom.h
        src/entities/level_2/sockenemy.cpp
        src/entities/level_2/sockenemy.h
        src/entities/level_2/sockshooter.cpp
        src/entities/level_2/sockshooter.h
        src/entities/level_1/pillowenemy.cpp
        src/entities/level_1/pillowenemy.h
        src/entities/level_2/pantsenemy.cpp
        src/entities/level_2/pantsenemy.h
        src/entities/projectile.cpp
        src/entities/projectile.h
        src/entities/level_2/toxicgas.cpp
        src/entities/level_2/toxicgas.h
        src/entities/level_2/orbitingsock.cpp
        src/entities/level_2/orbitingsock.h
        src/entities/level_2/walker.cpp
        src/entities/level_2/walker.h
        src/entities/usagi.cpp
        src/entities/usagi.h
        src/entities/level_3/scalingenemy.cpp
        src/entities/level_3/scalingenemy.h
        src/entities/level_3/optimizationenemy.cpp
        src/entities/level_3/optimizationenemy.h
        src/entities/level_3/digitalsystemenemy.cpp
        src/entities/level_3/digitalsystemenemy.h
        src/entities/level_3/yanglinenemy.cpp
        src/entities/level_3/yanglinenemy.h
        src/entities/level_3/zhuhaoenemy.cpp
        src/entities/level_3/zhuhaoenemy.h
        src/entities/level_3/xukeenemy.cpp
        src/entities/level_3/xukeenemy.h
        src/entities/level_3/probabilityenemy.cpp
        src/entities/level_3/probabilityenemy.h
)

set(WORLD_SOURCES
        src/world/level.cpp
        src/world/level.h
        src/world/room.cpp
        src/world/room.h
        src/world/door.cpp
        src/world/door.h
        src/world/levelconfig.cpp
        src/world/levelconfig.h
        src/world/factory/enemyfactory.cpp
        src/world/factory/enemyfactory.h
        src/world/factory/bossfactory.cpp
        src/world/factory/bossfactory.h
        src/world/roommanager.cpp
        src/world/roommanager.h
        src/world/bossfight.cpp
        src/world/bossfight.h
        src/world/rewardsystem.cpp
        src/world/rewardsystem.h
)

set(ITEM_SOURCES
        src/items/item.cpp
        src/items/item.h
        src/items/chest.cpp
        src/items/chest.h
        src/items/statuseffect.cpp
        src/items/statuseffect.h
        src/items/droppeditem.cpp
        src/items/droppeditem.h
        src/items/droppeditemfactory.cpp
        src/items/droppeditemfactory.h
        src/items/itemeffectconfig.cpp
        src/items/itemeffectconfig.h
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
        src/ui/explosion.h
        src/ui/explosion.cpp
        src/ui/characterselector.h
        src/ui/characterselector.cpp
        src/ui/pausemenu.h
        src/ui/pausemenu.cpp
        src/ui/dialogsystem.h
        src/ui/dialogsystem.cpp
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

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(game_final
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
            src/constants.h
    )
    # Define target properties for Android with Qt 6 as:
    #    set_property(TARGET game_final APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
    #                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
    # For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else ()
    if (ANDROID)
        add_library(game_final SHARED
                ${PROJECT_SOURCES}
        )
        # Define properties for Android with Qt 5 after find_package() calls as:
        #    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else ()
        add_executable(game_final
                ${PROJECT_SOURCES}
        )
    endif ()
endif ()

target_link_libraries(game_final PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Multimedia
)

if (MINGW)
    target_link_options(game_final PRIVATE -Wl,--allow-multiple-definition)
endif ()

# Disable qDebug() output in Release/RelWithDebInfo builds by defining the
# QT_NO_DEBUG_OUTPUT macro at compile-time. This will remove qDebug() logging
# calls from the binary, reducing noise and improving performance.
target_compile_definitions(game_final PRIVATE
        $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
        $<$<CONFIG:RelWithDebInfo>:QT_NO_DEBUG_OUTPUT>
)

# If user requested a Win32 GUI executable, handle platform-specific requirements.
if (BUILD_WIN32_EXECUTABLE)
    if (MSVC)
        # For MSVC, setting WIN32_EXECUTABLE is sufficient (handled above by CMAKE_WIN32_EXECUTABLE)
    elseif (MINGW)
        # MinGW may require extra mingw runtime libraries to satisfy Qt entry point symbols.
        # Append common mingw runtime libraries after Qt libs to help the linker resolve symbols.
        target_link_libraries(game_final PRIVATE -lmingwex -lmingwthrd)
        # Also set WIN32_EXECUTABLE property on the target so linker uses GUI subsystem
        set_target_properties(game_final PROPERTIES WIN32_EXECUTABLE TRUE)
    endif ()
endif ()

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
if (${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.game_final)
endif ()
set_target_properties(game_final PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
)

include(GNUInstallDirs)
install(TARGETS game_final
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 复制资源文件到构建目录（总是执行，确保资源更新）
add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:game_final>/assets
        COMMENT "Copying assets to build directory"
)
add_dependencies(game_final copy_assets)

# 复制 Qt 多媒体插件到构建目录（解决 "No QtMultimedia backends found" 问题）
if (WIN32 AND Qt6_FOUND)
    # 获取 Qt 安装目录
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    get_filename_component(QT_DIR "${QT_BIN_DIR}" DIRECTORY)

    # 复制多媒体插件
    add_custom_command(TARGET game_final POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:game_final>/multimedia"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QT_DIR}/plugins/multimedia"
            "$<TARGET_FILE_DIR:game_final>/multimedia"
            COMMENT "Copying Qt multimedia plugins"
    )

    # 复制 Qt6Multimedia.dll（如果存在）
    if (EXISTS "${QT_BIN_DIR}/Qt6Multimedia.dll")
        add_custom_command(TARGET game_final POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_BIN_DIR}/Qt6Multimedia.dll"
                "$<TARGET_FILE_DIR:game_final>/"
                COMMENT "Copying Qt6Multimedia.dll"
        )
    endif ()
endif ()

if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(game_final)
endif ()