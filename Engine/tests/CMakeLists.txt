# SPDX-License-Identifier: GPL-3.0-or-later

# --- Original OpenKey engine (frozen copy) + snapshot generator ---------------------------
# The original engine keeps its state in globals, so this target is only used to
# (re)generate and verify the snapshot; it is never linked into the app.
set(ORIG ${CMAKE_CURRENT_SOURCE_DIR}/original)
add_executable(gen_snapshot_original
  ${ORIG}/Engine.cpp ${ORIG}/Vietnamese.cpp ${ORIG}/Macro.cpp ${ORIG}/SmartSwitchKey.cpp ${ORIG}/ConvertTool.cpp
  ref/driver_original.cpp ref/gen_snapshot.cpp common/cases.cpp)
target_include_directories(gen_snapshot_original PRIVATE ${ORIG} ref)
# ConvertTool.cpp relies on <algorithm> being pulled in transitively (true on Apple clang only).
target_compile_options(gen_snapshot_original PRIVATE -include algorithm -w)

set(SNAPSHOT ${CMAKE_CURRENT_SOURCE_DIR}/snapshots/engine.snap)

# The committed snapshot must be exactly what the original engine produces.
add_test(NAME snapshot_matches_original_engine
  COMMAND ${CMAKE_COMMAND}
    -DGEN=$<TARGET_FILE:gen_snapshot_original> -DOUT=${CMAKE_CURRENT_BINARY_DIR}/original.snap
    -DEXPECTED=${SNAPSHOT} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/compare_snapshot.cmake)

# --- Refactored engine (C API) + Catch2 tests ---------------------------------------------
include(FetchContent)
FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(viskey_engine_tests snapshot_test.cpp api_test.cpp vk/driver_vk.cpp common/cases.cpp)
target_link_libraries(viskey_engine_tests PRIVATE viskey_engine Catch2::Catch2WithMain)
target_include_directories(viskey_engine_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_compile_definitions(viskey_engine_tests PRIVATE
  VK_SNAPSHOT_PATH="${SNAPSHOT}"
  VK_OVERRIDES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/snapshots/overrides.snap")
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(viskey_engine_tests)
