cmake_minimum_required(VERSION 3.15)
project(VisKey VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: static runtime (no vcruntime/msvcp DLLs next to VisKey64.exe) and UTF-8 sources.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/core)

# 1. Cross-platform CLI Simulator & Unit Test Suite
add_executable(viskey_cli core/viskey_cli.cpp)
add_executable(test_cpp_engine tests/test_cpp_engine.cpp)

enable_testing()
add_test(NAME VisKeyCoreTests COMMAND test_cpp_engine)

# Pure logic of the Windows update check (24 h throttle, notify once per version). No Windows
# headers, so it builds and runs on every platform.
add_executable(test_update_policy windows/tests/test_update_policy.cpp)
add_test(NAME VisKeyWindowsUpdatePolicyTests COMMAND test_update_policy)

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

# 3. Native Windows Win32 Application Target (VisKey64.exe)
if(WIN32)
    enable_language(RC)
    # Portable single-file exe. d2d1, dwrite, dwmapi and winhttp are loaded at runtime
    # (LoadLibraryW), WIC and the file dialogs go through COM (ole32), so the static imports stay
    # within the list checked by .github/workflows/windows.yml.
    add_executable(VisKey64 WIN32
        windows/VisKeyWin.cpp
        windows/ui/gfx.cpp
        windows/ui/widgets.cpp
        windows/ui/pages.cpp
        windows/ui/settings.cpp
        windows/ui/flyout.cpp
        windows/ui/store.cpp
        windows/ui/snapshot.cpp
        windows/res/VisKey.rc)
    target_compile_definitions(VisKey64 PRIVATE UNICODE _UNICODE NOMINMAX)
    # Icon + VERSIONINFO; the version comes from project(VERSION) above.
    set_source_files_properties(windows/res/VisKey.rc PROPERTIES COMPILE_DEFINITIONS
        "VISKEY_VERSION_MAJOR=${PROJECT_VERSION_MAJOR};VISKEY_VERSION_MINOR=${PROJECT_VERSION_MINOR};VISKEY_VERSION_PATCH=${PROJECT_VERSION_PATCH};VISKEY_VERSION_STR=\"${PROJECT_VERSION}\"")
    # Fallback for the version shown in the UI if the VERSIONINFO resource cannot be read.
    set_source_files_properties(windows/ui/store.cpp PROPERTIES COMPILE_DEFINITIONS "VISKEY_APP_VERSION=\"${PROJECT_VERSION}\"")
    target_link_libraries(VisKey64 PRIVATE user32 gdi32 shell32 advapi32 comctl32 ole32 uuid)
endif()
