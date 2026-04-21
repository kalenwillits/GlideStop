@echo off
setlocal enabledelayedexpansion

echo ========================================
echo  GlideStop X-Plane Plugin Build Script
echo ========================================
echo.

REM Check if we're in the right directory
if not exist "src\glidestop.cpp" (
    echo ERROR: This script must be run from the project root directory
    echo Current directory: %CD%
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build

REM Check for X-Plane SDK
set SDK_FOUND=0
if exist "SDK\CHeaders\XPLM\XPLMPlugin.h" (
    echo ✓ X-Plane SDK found in SDK directory
    set SDK_FOUND=1
) else if defined XPLANE_SDK_PATH (
    if exist "%XPLANE_SDK_PATH%\CHeaders\XPLM\XPLMPlugin.h" (
        echo ✓ X-Plane SDK found at XPLANE_SDK_PATH: %XPLANE_SDK_PATH%
        set SDK_FOUND=1
    )
)

if !SDK_FOUND! == 0 (
    echo.
    echo ❌ X-Plane SDK not found!
    echo.
    echo Please do one of the following:
    echo   1. Download SDK from https://developer.x-plane.com/sdk/plugin-sdk-downloads/
    echo   2. Extract it to the 'SDK' folder in this project directory
    echo   3. OR set XPLANE_SDK_PATH environment variable to SDK location
    echo.
    echo Expected file: SDK\CHeaders\XPLM\XPLMPlugin.h
    echo.
    pause
    exit /b 1
)

REM Detect Visual Studio via vswhere
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
    echo.
    echo ❌ Visual Studio not found!
    echo.
    echo Please install Visual Studio with the "Desktop development with C++" workload:
    echo   https://visualstudio.microsoft.com/downloads/
    echo.
    pause
    exit /b 1
)

set COMPILER_FOUND=0
for /f "tokens=1 delims=." %%v in ('%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion 2^>nul') do (
    if !COMPILER_FOUND! == 0 (
        set VS_MAJOR=%%v
        set COMPILER_FOUND=1
    )
)

if !COMPILER_FOUND! == 0 (
    echo.
    echo ❌ No Visual Studio installation found!
    echo.
    echo Please install Visual Studio with the "Desktop development with C++" workload:
    echo   https://visualstudio.microsoft.com/downloads/
    echo.
    pause
    exit /b 1
)

if "!VS_MAJOR!" == "18" (
    set BUILD_GENERATOR="Visual Studio 18 2026"
) else (
    set BUILD_GENERATOR="Visual Studio 17 2022"
)

echo ✅ Found !BUILD_GENERATOR!

REM Determine build type
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release

echo.
echo Build Configuration:
echo   Generator: !BUILD_GENERATOR!
echo   Build Type: %BUILD_TYPE%
echo.

REM Change to build directory
cd build

echo Configuring with !BUILD_GENERATOR!...
cmake .. -G !BUILD_GENERATOR! -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

if !errorlevel! neq 0 (
    echo.
    echo ❌ CMake configuration failed!
    echo.
    echo Troubleshooting:
    echo - Make sure Visual Studio C++ tools are properly installed
    echo - Reinstall Visual Studio with "Desktop development with C++" workload
    echo.
    pause
    cd ..
    exit /b 1
)

REM Build the project
echo.
echo Building plugin...
cmake --build . --config %BUILD_TYPE%

if !errorlevel! neq 0 (
    echo.
    echo ❌ Build failed!
    echo.
    echo Troubleshooting:
    echo - Make sure Visual Studio C++ tools are properly installed
    echo - Reinstall Visual Studio with "Desktop development with C++" workload
    echo.
    pause
    cd ..
    exit /b 1
)

REM Create plugin package
echo.
echo Creating plugin package...
cmake --build . --target plugin --config %BUILD_TYPE%
if !errorlevel! neq 0 (
    echo ❌ Plugin package creation failed
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo ✅ BUILD SUCCESSFUL!
echo ========================================
echo.
echo Plugin created at: build\GlideStop\win.xpl
echo.
echo To install:
echo 1. Copy the entire 'GlideStop' folder to X-Plane\Resources\plugins\
echo 2. Restart X-Plane
echo 3. GlideStop automatically controls brakes - no configuration needed
echo.

if "%1" neq "nobatch" pause

endlocal
