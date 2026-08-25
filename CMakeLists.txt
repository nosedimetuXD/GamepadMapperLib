cmake_minimum_required(VERSION 3.22)
project(GamepadMapper VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dynamic Qt6 Configuration
if(NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)
    if(DEFINED ENV{QTDIR})
        list(PREPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
    endif()
    # Search common Qt installation directories automatically
    file(GLOB QT_CANDIDATES
        "C:/Qt/6.*/msvc*_64"
        "D:/Qt/6.*/msvc*_64"
        "C:/Qt/Qt6.*/msvc*_64"
        "C:/Qt/*_msvc*_64"
    )
    if(QT_CANDIDATES)
        list(SORT QT_CANDIDATES ORDER DESCENDING)
        list(GET QT_CANDIDATES 0 LATEST_QT)
        list(PREPEND CMAKE_PREFIX_PATH "${LATEST_QT}")
        message(STATUS "Auto-detected Qt6 at: ${LATEST_QT}")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(FetchContent)

# Fetch fmt library
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.0.2
)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(fmt)

# Fetch SDL3
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.4
)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# Fetch standalone ASIO
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)

# Collect sources
file(GLOB_RECURSE COMMON_SOURCES
    "src/common/*.cpp"
)

file(GLOB_RECURSE CORE_SOURCES
    "src/core/*.cpp"
)

file(GLOB_RECURSE UI_SOURCES
    "src/ui/*.cpp"
)

file(GLOB_RECURSE UI_FORMS
    "src/ui/*.ui"
)

file(GLOB_RECURSE PUBLIC_HEADERS
    "include/GamepadMapper/*.h"
)

set(RESOURCES
    "res/icons/controller/controller.qrc"
    "res/icons/overlay/overlay.qrc"
    "res/app_icon.qrc"
)

add_library(GamepadMapperLib STATIC
    ${PUBLIC_HEADERS}
    ${COMMON_SOURCES}
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${UI_FORMS}
    ${RESOURCES}
    "src/GamepadManager.cpp"
    "src/GamepadConfigDialog.cpp"
    "src/SingleGamepadConfigDialog.cpp"
    "src/SingleSwitchConfigDialog.cpp"
)

target_include_directories(GamepadMapperLib PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)

target_include_directories(GamepadMapperLib PRIVATE
    "src"
    "src/common"
    "src/core"
    "src/ui"
    "externals"
    "${asio_SOURCE_DIR}/asio/include"
)

target_link_libraries(GamepadMapperLib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    fmt::fmt
    SDL3::SDL3-shared
    ntdll
)

if(MSVC)
    target_compile_definitions(GamepadMapperLib PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _UNICODE
        UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
        ASIO_STANDALONE
        HAVE_SDL3
    )
    target_compile_options(GamepadMapperLib PRIVATE
        /utf-8
        /permissive-
        /Zc:preprocessor
        /W3
    )
endif()

# Standalone Multi-Player Executable Application
add_executable(GamepadMapperApp WIN32
    "examples/standalone_app/main.cpp"
    "res/app_icon.rc"
)
target_link_libraries(GamepadMapperApp PRIVATE GamepadMapperLib)

# Single Player Remapper Executable Application (FC 26 style)
add_executable(SingleGamepadMapperApp WIN32
    "examples/single_player_mapper/main.cpp"
    "res/app_icon.rc"
)
target_link_libraries(SingleGamepadMapperApp PRIVATE GamepadMapperLib)

# Single Switch Remapper Executable Application (Switch Target)
add_executable(SingleSwitchMapperApp WIN32
    "examples/single_switch_mapper/main.cpp"
    "res/app_icon.rc"
)
target_link_libraries(SingleSwitchMapperApp PRIVATE GamepadMapperLib)

# C-ABI Shared DLL for Python (ctypes) & external bindings
add_library(GamepadMapper SHARED
    "src/GamepadMapperC.cpp"
    "include/GamepadMapper/GamepadMapperC.h"
)
target_include_directories(GamepadMapper PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>"
)
target_compile_definitions(GamepadMapper PRIVATE GAMEPADMAPPER_EXPORTS)
target_link_libraries(GamepadMapper PRIVATE
    GamepadMapperLib
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    SDL3::SDL3-shared
)

