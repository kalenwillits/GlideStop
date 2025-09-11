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

REM Detect available compilers
set COMPILER_FOUND=0
set BUILD_GENERATOR=""
set CMAKE_EXTRA_FLAGS=""

REM Check for Visual Studio 2022
where cl >nul 2>&1
if !errorlevel! == 0 (
    echo ✓ Visual Studio compiler found
    set BUILD_GENERATOR="Visual Studio 17 2022"
    set CMAKE_EXTRA_FLAGS=-A x64
    set COMPILER_FOUND=1
    goto :compiler_detected
)

REM Check for Visual Studio 2019
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019" (
    echo ✓ Visual Studio 2019 found
    set BUILD_GENERATOR="Visual Studio 16 2019"
    set CMAKE_EXTRA_FLAGS=-A x64
    set COMPILER_FOUND=1
    goto :compiler_detected
)

REM Check for MinGW
where g++ >nul 2>&1
if !errorlevel! == 0 (
    echo ✓ MinGW compiler found
    set BUILD_GENERATOR="MinGW Makefiles"
    set COMPILER_FOUND=1
    goto :compiler_detected
)

:compiler_detected
if !COMPILER_FOUND! == 0 (
    echo.
    echo ❌ No compatible compiler found!
    echo.
    echo Please install one of the following:
    echo   - Visual Studio 2019 or 2022 with C++ support
    echo   - MinGW-w64
    echo   - MSYS2 with mingw-w64-x86_64-gcc
    echo.
    pause
    exit /b 1
)

REM Determine build type
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release

echo.
echo Build Configuration:
echo   Generator: %BUILD_GENERATOR%
echo   Build Type: %BUILD_TYPE%
echo   Extra Flags: %CMAKE_EXTRA_FLAGS%
echo.

REM Change to build directory
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -G %BUILD_GENERATOR% %CMAKE_EXTRA_FLAGS% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if !errorlevel! neq 0 (
    echo ❌ CMake configuration failed
    cd ..
    pause
    exit /b 1
)

REM Build the project
echo.
echo Building GlideStop plugin...
cmake --build . --config %BUILD_TYPE%
if !errorlevel! neq 0 (
    echo ❌ Build failed
    cd ..
    pause
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