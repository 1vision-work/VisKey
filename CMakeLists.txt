cmake_minimum_required(VERSION 3.15)
project(VisKey VERSION 1.0.0 LANGUAGES CXX)

# Full release version (e.g. 1.1.0-beta.5 from tag v1.1.0-beta.5; CI passes the output of
# scripts/viskey_version.sh). Defaults to project(VERSION) for local, untagged builds.
set(VISKEY_VERSION "${PROJECT_VERSION}" CACHE STRING "VisKey version: x.y.z[-prerelease][+build]")
if(NOT VISKEY_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-[0-9A-Za-z.-]+)?(\\+[0-9A-Za-z.-]+)?$")
    message(FATAL_ERROR "VISKEY_VERSION '${VISKEY_VERSION}' is not x.y.z[-prerelease][+build]")
endif()
set(VISKEY_VERSION_MAJOR ${CMAKE_MATCH_1})
set(VISKEY_VERSION_MINOR ${CMAKE_MATCH_2})
set(VISKEY_VERSION_PATCH ${CMAKE_MATCH_3})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: static runtime (no vcruntime/msvcp DLLs next to VisKey.exe) and UTF-8 sources.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/core)

# 1. Cross-platform CLI Simulator & Unit Test Suite
add_executable(viskey_cli core/viskey_cli.cpp)
add_executable(test_cpp_engine tests/test_cpp_engine.cpp)
# Platform-stream typing regression (cases in tests/data/typing_regression.tsv)
add_executable(test_typing_regression tests/test_typing_regression.cpp)
target_compile_definitions(test_typing_regression PRIVATE VISKEY_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
# Pure C++ policy of the macOS automatic update check (header-only, builds on every platform).
add_executable(test_update_policy tests/test_update_policy.cpp)
# Pure logic of the Windows update check (24 h throttle, notify once per version). No Windows
# headers, so it builds and runs on every platform.
add_executable(test_update_policy_win windows/tests/test_update_policy.cpp)

enable_testing()
add_test(NAME VisKeyCoreTests COMMAND test_cpp_engine)
add_test(NAME VisKeyTypingRegression COMMAND test_typing_regression)
add_test(NAME VisKeyUpdatePolicyTests COMMAND test_update_policy)
add_test(NAME VisKeyWindowsUpdatePolicyTests COMMAND test_update_policy_win)

# 2. Native macOS App Bundle Target
if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    # Keep this list in sync with scripts/build_macos_installer.sh.
    set(VISKEY_MACOS_SOURCES
        macos/VisKeyApp.mm
        macos/ui/VKUIKit.mm
        macos/ui/VKModel.mm
        macos/ui/VKPageBase.mm
        macos/ui/VKPageGeneral.mm
        macos/ui/VKPageMacros.mm
        macos/ui/VKPageApps.mm
        macos/ui/VKPageShortcuts.mm
        macos/ui/VKPageInfo.mm
        macos/ui/VKSettingsWindow.mm
        macos/ui/VKPopover.mm
        macos/ui/VKHUD.mm
        macos/ui/VKOnboarding.mm
        macos/ui/VKUpdate.mm
    )
    add_executable(VisKey MACOSX_BUNDLE ${VISKEY_MACOS_SOURCES})
    set_target_properties(VisKey PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist"
        COMPILE_FLAGS "-fobjc-arc"
    )
    # Match LSMinimumSystemVersion (11.0) in macos/Info.plist.
    target_compile_options(VisKey PRIVATE -mmacosx-version-min=11.0 -Wall)
    target_link_options(VisKey PRIVATE -mmacosx-version-min=11.0)
    target_link_libraries(VisKey
        "-framework Cocoa"
        "-framework ApplicationServices"
        "-framework Carbon"
    )
endif()

# 3. Native Windows Win32 Application Target (VisKey.exe)
if(WIN32)
    enable_language(RC)
    # Portable single-file exe. d2d1, dwrite, dwmapi and winhttp are loaded at runtime
    # (LoadLibraryW), WIC and the file dialogs go through COM (ole32), so the static imports stay
    # within the list checked by .github/workflows/windows.yml.
    add_executable(VisKey WIN32
        windows/VisKeyWin.cpp
        windows/ui/gfx.cpp
        windows/ui/widgets.cpp
        windows/ui/pages.cpp
        windows/ui/settings.cpp
        windows/ui/flyout.cpp
        windows/ui/store.cpp
        windows/ui/snapshot.cpp
        windows/res/VisKey.rc)
    target_compile_definitions(VisKey PRIVATE UNICODE _UNICODE NOMINMAX)
    # Icon + VERSIONINFO; the version comes from VISKEY_VERSION above (numeric x.y.z in the fixed
    # part, the full string in FileVersion/ProductVersion).
    set_source_files_properties(windows/res/VisKey.rc PROPERTIES COMPILE_DEFINITIONS
        "VISKEY_VERSION_MAJOR=${VISKEY_VERSION_MAJOR};VISKEY_VERSION_MINOR=${VISKEY_VERSION_MINOR};VISKEY_VERSION_PATCH=${VISKEY_VERSION_PATCH};VISKEY_VERSION_STR=\"${VISKEY_VERSION}\"")
    # Version shown in the UI (full string, so pre-releases read e.g. "1.1.0-beta.5").
    set_source_files_properties(windows/ui/store.cpp PROPERTIES COMPILE_DEFINITIONS "VISKEY_APP_VERSION=\"${VISKEY_VERSION}\"")
    target_link_libraries(VisKey PRIVATE user32 gdi32 shell32 advapi32 comctl32 ole32 uuid)
endif()
