# SPDX-License-Identifier: GPL-3.0-or-later
#
# VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
#
# Builds a portable, self-contained VisKey.exe: Win32 C++17, static MSVC runtime (/MT), icons + manifest +
# version info embedded, no DLL dependency beyond Windows system DLLs.
#
#   cmake -S windows -B build -A x64      # newest installed Visual Studio generator (or pass -G explicitly)
#   cmake --build build --config Release
#   ctest --test-dir build -C Release
cmake_minimum_required(VERSION 3.20)

# Static runtime for every target, including the engine library (must be set before project()).
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(VisKeyWindows VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VISKEY_BUILD_TESTS "Build the Engine/ Catch2 tests (they use macOS key codes; not run on Windows)" OFF)
option(VISKEY_WINDOWS_TESTS "Build windows/tests" ON)

# ---- engine -------------------------------------------------------------------------------------
# Engine/ is used as is. On _WIN32 it includes <windows.h> (Engine/src/platforms/win32.h) so the engine key
# codes are Windows virtual-key codes. Flags it needs under MSVC are set here, not in Engine/:
#  - NOMINMAX: viskey_engine.cpp calls std::min(a, b), which the min() macro of <windows.h> would break;
#  - WIN32_LEAN_AND_MEAN: DataType.h has `using namespace std;` before <windows.h>; the full header pulls in
#    rpcndr.h whose `byte` then clashes with std::byte (C2872);
#  - the C++17 <codecvt> deprecation warning (Engine.h/ConvertTool.cpp use wstring_convert).
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../Engine ${CMAKE_BINARY_DIR}/engine)
target_compile_definitions(viskey_engine PRIVATE
  NOMINMAX WIN32_LEAN_AND_MEAN _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING _CRT_SECURE_NO_WARNINGS)
if(MSVC)
  target_compile_options(viskey_engine PRIVATE /utf-8 /W3 /wd4244 /wd4267 /wd4018 /wd4996)
endif()

# ---- app logic (no UI, unit-tested) --------------------------------------------------------------
set(WIN_COMMON_DEFS UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00 WINVER=0x0A00)
configure_file(src/version.h.in ${CMAKE_BINARY_DIR}/generated/version.h @ONLY)

add_library(viskey_win_core STATIC
  src/json.cpp src/config.cpp src/keymap.cpp src/output_planner.cpp src/win_util.cpp)
target_include_directories(viskey_win_core PUBLIC src ${CMAKE_BINARY_DIR}/generated)
target_link_libraries(viskey_win_core PUBLIC viskey_engine shell32 ole32 advapi32 user32)
target_compile_definitions(viskey_win_core PUBLIC ${WIN_COMMON_DEFS})
if(MSVC)
  target_compile_options(viskey_win_core PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:/utf-8> $<$<COMPILE_LANGUAGE:CXX>:/W3> $<$<COMPILE_LANGUAGE:CXX>:/permissive->)
endif()

# ---- VisKey.exe ---------------------------------------------------------------------------------
add_executable(VisKey WIN32
  src/app.cpp src/dialogs.cpp src/typing_service.cpp
  res/VisKey.rc res/VisKey.manifest)
target_include_directories(VisKey PRIVATE res)
target_link_libraries(VisKey PRIVATE viskey_win_core comctl32 gdi32)
set_target_properties(VisKey PROPERTIES OUTPUT_NAME VisKey)
if(MSVC)
  # res/VisKey.manifest (a source of the target) is embedded by CMake; it already holds the
  # requestedExecutionLevel, so the linker must not add its own UAC fragment.
  target_link_options(VisKey PRIVATE /MANIFESTUAC:NO)
endif()

# ---- tests --------------------------------------------------------------------------------------
if(VISKEY_WINDOWS_TESTS)
  enable_testing()
  add_executable(win_keymap_test tests/win_keymap_test.cpp)
  target_link_libraries(win_keymap_test PRIVATE viskey_win_core)
  add_test(NAME win_keymap_test COMMAND win_keymap_test)
endif()
