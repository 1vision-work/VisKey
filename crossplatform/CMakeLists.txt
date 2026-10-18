# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.14)
project(VisKey VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/core)

# 1. Cross-platform CLI Simulator & Unit Test Binary
add_executable(viskey_cli core/viskey_cli.cpp)
add_executable(test_cpp_engine tests/test_cpp_engine.cpp)

enable_testing()
add_test(NAME VisKeyCoreTests COMMAND test_cpp_engine)

# 2. Native macOS App Bundle Target
if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(VISKEY_BRANDING_DIR "${CMAKE_CURRENT_SOURCE_DIR}/branding/macos")
    set(VISKEY_RESOURCES
        "${VISKEY_BRANDING_DIR}/VisKey.icns"
        "${VISKEY_BRANDING_DIR}/menubar-vi.png"
        "${VISKEY_BRANDING_DIR}/menubar-vi@2x.png"
        "${VISKEY_BRANDING_DIR}/menubar-en.png"
        "${VISKEY_BRANDING_DIR}/menubar-en@2x.png"
        "${VISKEY_BRANDING_DIR}/menubar-off.png"
        "${VISKEY_BRANDING_DIR}/menubar-off@2x.png"
    )
    set_source_files_properties(${VISKEY_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    add_executable(VisKey MACOSX_BUNDLE macos/VisKeyApp.mm ${VISKEY_RESOURCES})
    set_target_properties(VisKey PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist"
        MACOSX_BUNDLE_ICON_FILE VisKey
        COMPILE_FLAGS "-fobjc-arc"
    )
    target_link_libraries(VisKey
        "-framework Cocoa"
        "-framework ApplicationServices"
        "-framework Carbon"
    )
endif()
