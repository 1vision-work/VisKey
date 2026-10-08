# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)
project(viskey_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The engine itself: C++17, no platform code, no global state. Exposed through include/viskey_engine.h.
add_library(viskey_engine STATIC
  src/Engine.cpp src/Vietnamese.cpp src/Macro.cpp src/SmartSwitchKey.cpp src/ConvertTool.cpp src/viskey_engine.cpp)
target_include_directories(viskey_engine PUBLIC include PRIVATE src)

option(VISKEY_BUILD_TESTS "Build engine tests" ON)

if(VISKEY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
