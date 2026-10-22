@echo off
chcp 65001 >nul
cd /d "%~dp0"
if exist "VisKey.exe" (
    start "" "VisKey.exe"
    exit /b 0
)
where pythonw >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    start "" pythonw "windows\VisKey_Windows_Portable.pyw"
    exit /b 0
)
call Build_Windows.bat
