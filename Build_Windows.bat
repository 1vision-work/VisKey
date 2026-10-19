@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo ====================================================================
echo   VisKey - Trinh Build Bo Cai Dat Native Win32 C++17 (VisKey64.exe)
echo ====================================================================

where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [1/2] Phat hien trinh bien dich MinGW-w64 g++. Dang chay kiem thu 109 Unit Tests...
    g++ -std=c++17 -O2 tests\test_cpp_engine.cpp -o test_cpp_engine.exe
    test_cpp_engine.exe
    del test_cpp_engine.exe
    echo [2/2] Dang bien dich VisKey64.exe Native Win32...
    g++ -std=c++17 -O2 -mwindows -municode windows\VisKeyWin.cpp -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32 -o VisKey64.exe
    echo.
    echo [HOAN TAT] Da tao file chay doc lap: VisKey64.exe!
    start "" VisKey64.exe
    pause
    exit /b 0
)

where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [1/2] Phat hien trinh bien dich MSVC cl.exe. Dang bien dich VisKey64.exe...
    cl /std:c++17 /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE windows\VisKeyWin.cpp user32.lib gdi32.lib shell32.lib advapi32.lib comctl32.lib /link /SUBSYSTEM:WINDOWS /OUT:VisKey64.exe
    echo.
    echo [HOAN TAT] Da tao file chay doc lap: VisKey64.exe!
    start "" VisKey64.exe
    pause
    exit /b 0
)

where python >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [!] May chua cai MinGW g++ hoac Visual Studio C++.
    echo [-> Chuyen sang khoi dong truc tiep ban VisKey Portable bang Python!]
    start "" pythonw windows\VisKey_Windows_Portable.pyw
    exit /b 0
)

echo [!] Vui long cai dat MinGW-w64, Visual Studio Build Tools, hoac Python 3.
pause
