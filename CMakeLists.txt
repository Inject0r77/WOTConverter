cmake_minimum_required(VERSION 3.24)

# Use CMake's MSVC runtime abstraction so release binaries can be portable
# without requiring the Microsoft Visual C++ Redistributable.
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(WoTConverter VERSION 1.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# All project sources are UTF-8. MSVC otherwise uses the active Windows code page
# and turns Cyrillic wide-string literals into mojibake (e.g. "РљР...").
if(MSVC)
    add_compile_options(/utf-8)
    # Release/RelWithDebInfo/MinSizeRel: /MT, Debug: /MTd.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(WOT_BUILD_TESTS "Build core tests" ON)

add_library(wot_core STATIC
    src/core/Converter.cpp
    src/core/BoxOptimizer.cpp
    src/core/Rates.cpp
)

target_include_directories(wot_core PUBLIC src)

if(MSVC)
    target_compile_options(wot_core PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(wot_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    set(WOT_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.ico")
    configure_file(resources/WoTConverter.rc.in "${CMAKE_CURRENT_BINARY_DIR}/WoTConverter.rc" @ONLY)

    add_executable(WoTConverter WIN32
        src/app/WinMain.cpp
        src/ui/MainWindow.cpp
        src/ui/Theme.cpp
        src/storage/Settings.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/WoTConverter.rc"
    )
    target_include_directories(WoTConverter PRIVATE src resources)
    target_link_libraries(WoTConverter PRIVATE
        wot_core
        user32 gdi32 gdiplus comctl32 shcore dwmapi uxtheme shell32
    )
    target_compile_definitions(WoTConverter PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    if(MSVC)
        target_compile_options(WoTConverter PRIVATE /W4 /permissive- /EHsc)
    endif()

    add_custom_command(TARGET WoTConverter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:WoTConverter>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/config/rates.json"
                "$<TARGET_FILE_DIR:WoTConverter>/config/rates.json"
    )
endif()

if(WOT_BUILD_TESTS)
    enable_testing()
    add_executable(wot_core_tests tests/CoreTests.cpp)
    target_link_libraries(wot_core_tests PRIVATE wot_core)
    target_compile_definitions(wot_core_tests PRIVATE WOT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME wot_core_tests COMMAND wot_core_tests)
endif()
