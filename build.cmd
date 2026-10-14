@echo off
rem Command-line build of the CMM2 firmware (same presets VS Code uses).
rem Usage: build [Release|Debug] [clean]      default is Release
setlocal
cd /d "%~dp0"

set PRESET=%~1
if "%PRESET%"=="" set PRESET=Release

if /i "%~2"=="clean" if exist "build\%PRESET%" rmdir /s /q "build\%PRESET%"

cmake --preset %PRESET% || exit /b 1
cmake --build --preset %PRESET% || exit /b 1

echo.
echo Output: %~dp0build\%PRESET%\CMM2V1.5.bin
