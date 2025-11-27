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
        src/core/gamecontroller.cpp
        src/core/gamecontroller.h
        src/core/gamemanager.cpp
        src/core/gamemanager.h
        src/core/audiomanager.cpp
        src/core/audiomanager.h
        src/core/resourcefactory.h
        src/core/configmanager.cpp
        src/core/configmanager.h
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
        src/entities/nightmareboss.cpp
        src/entities/nightmareboss.h
        src/entities/washmachineboss.cpp
        src/entities/washmachineboss.h
        src/entities/teacherboss.cpp
        src/entities/teacherboss.h
        src/entities/invigilator.cpp
        src/entities/invigilator.h
        src/entities/chalkbeam.cpp
        src/entities/chalkbeam.h
        src/entities/exampaper.cpp
        src/entities/exampaper.h
        src/entities/mletrap.cpp
        src/entities/mletrap.h
        src/entities/clockenemy.cpp
        src/entities/clockenemy.h
        src/entities/clockboom.cpp
        src/entities/clockboom.h
        src/entities/sockenemy.cpp
        src/entities/sockenemy.h
        src/entities/sockshooter.cpp
        src/entities/sockshooter.h
        src/entities/pillowenemy.cpp
        src/entities/pillowenemy.h
        src/entities/pantsenemy.cpp
        src/entities/pantsenemy.h
        src/entities/projectile.cpp
        src/entities/projectile.h
        src/entities/toxicgas.cpp
        src/entities/toxicgas.h
        src/entities/orbitingsock.cpp
        src/entities/orbitingsock.h
        src/entities/walker.cpp
        src/entities/walker.h
        src/entities/usagi.cpp
        src/entities/usagi.h
)

set(WORLD_SOURCES
        src/world/level.cpp
        src/world/level.h
        src/world/room.cpp
        src/world/room.h
        src/world/door.cpp
        src/world/door.h
        src/world/randomgenerator.cpp
        src/world/randomgenerator.h
        src/world/levelconfig.cpp
        src/world/levelconfig.h
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
        src/ui/explosion.h
        src/ui/explosion.cpp
        src/ui/characterselector.h
        src/ui/characterselector.cpp
        src/ui/pausemenu.h
        src/ui/pausemenu.cpp
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
        src/world/wall.h
        src/world/map.h
        src/world/map.cpp
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
