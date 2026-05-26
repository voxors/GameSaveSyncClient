cmake_minimum_required(VERSION 3.16)

include(FetchContent)
cmake_policy(SET CMP0135 NEW)
set (CMAKE_COLOR_DIAGNOSTICS ON)

set(TARGET_NAME GameSaveSyncClient)

set(QT_MAJOR_VERSION 6)

project(${TARGET_NAME} VERSION 0.6.1 DESCRIPTION "GameSaveSyncClient")

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
    src/autoPathFinderUtils.cpp
    src/autoPathFinderUtils.h
    src/backgroundServerWorker.cpp
    src/backgroundServerWorker.h
    src/backgroundSyncWorker.cpp
    src/backgroundSyncWorker.h
    src/config.cpp
    src/config.h
    src/detailsViewWidget.cpp
    src/detailsViewWidget.h
    src/error.h
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
    src/trayIcon.cpp
    src/trayIcon.h
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

qt_add_resources(${TARGET_NAME} "license"
    PREFIX "/"
    FILES
        res/license/LICENSE.txt
        res/license/Third-Party.txt
)

target_compile_definitions(${TARGET_NAME} PRIVATE
  $<$<CONFIG:Debug>:DEBUG QT_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
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

target_compile_options(miniz_static INTERFACE
  $<$<CXX_COMPILER_ID:Clang>:-Wno-unused-function>
  $<$<CXX_COMPILER_ID:GNU>:-Wno-unused-function>
  $<$<CXX_COMPILER_ID:MSVC>:/wd4505>
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

install(TARGETS ${TARGET_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES ${CMAKE_SOURCE_DIR}/res/GameSaveSyncClient.desktop
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)
install(FILES ${CMAKE_SOURCE_DIR}/res/icon/GameSaveSyncClient.svg
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
)
install(FILES ${CMAKE_SOURCE_DIR}/res/license/LICENSE.txt
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${TARGET_NAME}
)
install(FILES ${CMAKE_SOURCE_DIR}/res/license/Third-Party.txt
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/${TARGET_NAME}
)

set(CPACK_PACKAGE_NAME ${TARGET_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "GameSaveSyncClient - A cross-platform Qt6 client for synchronizing game save files.")
set(CPACK_PACKAGE_DESCRIPTION
    "GameSaveSyncClient is a lightweight Qt6-based desktop application that helps you \
     synchronize and back up your game save files across systems. It supports multiple \
     platforms, provides a tray icon for background sync, and integrates with \
     configurable game paths for easy management.")
set(CPACK_PACKAGE_VENDOR "Jonathan Croteau-Dicaire")
set(CPACK_PACKAGE_CONTACT "jonathan.croteau.dicaire@gmail.com")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/voxors/GameSaveSyncClient")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/res/license/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/res/icon/GameSaveSyncClient.svg")

set(CPACK_GENERATOR "DEB;RPM;TGZ")

#DEB
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Jonathan Croteau-Dicaire")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6 (>= 6.2), libqt6gui6 (>= 6.2), libqt6widgets6 (>= 6.2), \
     libqt6svg6 (>= 6.2), libqt6svgwidgets6 (>= 6.2)")
#RPM
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Utilities")
set(CPACK_RPM_PACKAGE_RELEASE 2)
set(CPACK_RPM_PACKAGE_REQUIRES
    "qt6-qtbase >= 6.2, qt6-qtsvg >= 6.2")
# For openSUSE
# set(CPACK_RPM_PACKAGE_REQUIRES
#   "libQt6Core6 >= 6.2, libQt6Gui6 >= 6.2, \
#    libQt6SvgWidgets6 >= 6.2, libQt6Network6 >= 6.2, libQt6Svg6 >= 6.2")

set(CPACK_INSTALL_PREFIX "/usr")

include(CPack)
