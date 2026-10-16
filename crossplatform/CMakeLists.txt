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
    add_executable(VisKey MACOSX_BUNDLE macos/VisKeyApp.mm)
    set_target_properties(VisKey PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist"
        COMPILE_FLAGS "-fobjc-arc"
    )
    target_link_libraries(VisKey
        "-framework Cocoa"
        "-framework ApplicationServices"
        "-framework Carbon"
    )
endif()
