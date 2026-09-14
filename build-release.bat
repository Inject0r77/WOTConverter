@echo off
setlocal
cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found in PATH.
    echo Install Visual Studio 2022 with Desktop development with C++ and CMake tools.
    pause
    exit /b 1
)

echo [1/3] Configuring x64 Release build...
cmake -S . -B build -A x64 -DWOT_BUILD_TESTS=ON
if errorlevel 1 goto :fail

echo [2/3] Building...
cmake --build build --config Release --parallel
if errorlevel 1 goto :fail

echo [3/3] Running core tests...
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto :fail

echo.
echo Done.
echo EXE: %CD%\build\Release\WoTConverter.exe
pause
exit /b 0

:fail
echo.
echo [ERROR] Build failed.
pause
exit /b 1
