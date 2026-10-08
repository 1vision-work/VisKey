# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)
project(viskey_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VISKEY_BUILD_TESTS "Build engine tests" ON)

if(VISKEY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
