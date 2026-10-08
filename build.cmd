@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build

rem Use a project-local compiler without changing the system PATH.
if not exist ".tools\w64devkit\bin\gcc.exe" goto missing
".tools\w64devkit\bin\gcc.exe" -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O0 main.c -o build\main.exe
exit /b %errorlevel%

:missing
echo Portable GCC is missing. Run: powershell -ExecutionPolicy Bypass -File setup.ps1
exit /b 2
