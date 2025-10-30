cmake_minimum_required(VERSION 3.16)

include(FetchContent)
cmake_policy(SET CMP0135 NEW)

set(TARGET_NAME GameSaveSyncClient)

set(QT_MAJOR_VERSION 6)

project(${TARGET_NAME} VERSION 0.1 DESCRIPTION "GameSaveSyncClient")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#See if the environment var is set
if(DEFINED ENV{Qt${QT_MAJOR_VERSION}_HOME})
    message(STATUS "Looking for Qt in: " $ENV{Qt${QT_MAJOR_VERSION}_HOME})
else()
    message(STATUS
        "Qt${QT_MAJOR_VERSION}
        _HOME environment variable not set. Checking default paths.")
endif()

find_package(Qt${QT_MAJOR_VERSION} COMPONENTS
    Widgets Network Svg SvgWidgets
    REQUIRED PATHS $ENV{Qt${QT_MAJOR_VERSION}_HOME})

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release"
      CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)"
      FORCE
  )
endif()

add_executable(${TARGET_NAME}
    src/aboutDialog.cpp
    src/aboutDialog.h
    src/addGameDialog.cpp
    src/addGameDialog.h
    src/backgroundSyncWorker.cpp
    src/backgroundSyncWorker.h
    src/config.cpp
    src/config.h
    src/detailsViewWidget.cpp
    src/detailsViewWidget.h
    src/main.cpp
    src/mainWindow.cpp
    src/mainWindow.h
    src/pathEditableRowWidget.cpp
    src/pathEditableRowWidget.h
    src/pathItemDelegate.cpp
    src/pathItemDelegate.h
    src/pathListModel.cpp
    src/pathListModel.h
    src/setupWindow.cpp
    src/setupWindow.h
    src/status.h
    src/utilFileSystem.cpp
    src/utilFileSystem.h
    src/utilGameSyncServer.cpp
    src/utilGameSyncServer.h
)

qt_add_resources(${TARGET_NAME} "icons"
    PREFIX "/"
    FILES
        res/icon/GameSaveSyncClient.svg
        res/icon/GameSaveSyncClientTray.svg
)

target_compile_definitions(${TARGET_NAME} PRIVATE
  $<$<CONFIG:Debug>:DEBUG QT_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
  $<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
  $<$<PLATFORM_ID:Linux>:PLATFORM_LINUX>
)

if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE
    $<$<CONFIG:Debug>:/Zi /Od>
    $<$<CONFIG:Release>:/O2>
    /W4 /WX /wd4458
  )
else()
    target_compile_options(${TARGET_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Release>:-O3>
    -Wall -Wextra -pedantic -Werror
  )
endif()

target_include_directories(
    ${TARGET_NAME} PRIVATE ${Qt${QT_MAJOR_VERSION}_INCLUDE_DIRS}
    ${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(${TARGET_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin/Debug"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

FetchContent_Declare(miniz
    URL
    https://github.com/richgel999/miniz/releases/download/3.1.0/miniz-3.1.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(miniz)
add_library(miniz_static STATIC
    "${miniz_SOURCE_DIR}/miniz.c"
)
target_include_directories(miniz_static
    PUBLIC SYSTEM "${miniz_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(miniz_static PRIVATE /W0)
else()
    target_compile_options(miniz_static PRIVATE -w)
endif()

target_link_libraries(${TARGET_NAME}
    PRIVATE
        miniz_static
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Svg
        Qt${QT_MAJOR_VERSION}::SvgWidgets
)
