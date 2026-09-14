@echo off
setlocal
cd /d "%~dp0"

title WOTConverter Release Build

echo ==========================================
echo   WOTConverter - Release Build
echo ==========================================
echo.

rem -------------------------------------------------
rem Check whether CMake is already available
rem -------------------------------------------------
where cmake >nul 2>&1
if not errorlevel 1 goto :cmake_ready

echo [INFO] CMake is not currently available in PATH.
echo [INFO] Trying to initialize Visual Studio environment...
echo.

rem -------------------------------------------------
rem Locate Visual Studio via vswhere
rem -------------------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo [ERROR] Visual Studio Installer / vswhere.exe was not found.
    echo.
    echo Install Visual Studio 2022 with:
    echo   Desktop development with C++
    echo   CMake tools for Windows
    echo.
    pause
    exit /b 1
)

set "VSINSTALL="

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VSINSTALL=%%i"
)

if not defined VSINSTALL (
    echo [ERROR] Visual Studio 2022 C++ toolchain was not found.
    echo.
    echo Install the "Desktop development with C++" workload.
    echo.
    pause
    exit /b 1
)

set "VSDEVCMD=%VSINSTALL%\Common7\Tools\VsDevCmd.bat"

if not exist "%VSDEVCMD%" (
    echo [ERROR] VsDevCmd.bat was not found:
    echo %VSDEVCMD%
    pause
    exit /b 1
)

call "%VSDEVCMD%" -arch=x64 -host_arch=x64 >nul

rem -------------------------------------------------
rem Check CMake again after VS environment setup
rem -------------------------------------------------
where cmake >nul 2>&1

if errorlevel 1 (
    echo [ERROR] CMake was still not found.
    echo.
    echo Open Visual Studio Installer and install:
    echo   Desktop development with C++
    echo   CMake tools for Windows
    echo.
    pause
    exit /b 1
)

:cmake_ready

echo [OK] CMake found:
where cmake
echo.

rem -------------------------------------------------
rem Clean previous build
rem -------------------------------------------------
if exist build (
    echo [INFO] Removing previous build directory...
    rmdir /s /q build
)

echo.
echo [1/3] Configuring...
cmake -S . -B build -A x64
if errorlevel 1 goto :build_failed

echo.
echo [2/3] Building Release...
cmake --build build --config Release --parallel
if errorlevel 1 goto :build_failed

echo.
echo [3/3] Running tests...
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 goto :build_failed

echo.
echo ==========================================
echo   BUILD SUCCESSFUL
echo ==========================================
echo.
echo Executable:
echo   build\Release\WoTConverter.exe
echo.
pause
exit /b 0

:build_failed
echo.
echo ==========================================
echo   BUILD FAILED
echo ==========================================
echo.
pause
exit /b 1
