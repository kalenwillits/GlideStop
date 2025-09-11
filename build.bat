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

echo Detecting available compilers...

REM 1. Try Visual Studio 2022 first
where /q devenv 2>nul
if !errorlevel! == 0 (
    for /f "tokens=*" %%i in ('where devenv 2^>nul') do (
        if "!BUILD_GENERATOR!" == """" (
            echo %%i | findstr /i "2022" >nul
            if !errorlevel! == 0 (
                set BUILD_GENERATOR="Visual Studio 17 2022"
                set CMAKE_EXTRA_FLAGS=-A x64
                set COMPILER_FOUND=1
                echo ✅ Found Visual Studio 2022
            )
        )
    )
)

REM 2. Try Visual Studio 2019
if !COMPILER_FOUND! == 0 (
    for /f "tokens=*" %%i in ('where devenv 2^>nul') do (
        if "!BUILD_GENERATOR!" == """" (
            echo %%i | findstr /i "2019" >nul
            if !errorlevel! == 0 (
                set BUILD_GENERATOR="Visual Studio 16 2019"
                set CMAKE_EXTRA_FLAGS=-A x64
                set COMPILER_FOUND=1
                echo ✅ Found Visual Studio 2019
            )
        )
    )
)

REM 3. Try Visual Studio Build Tools (lighter than full VS)
if !COMPILER_FOUND! == 0 (
    where /q cl 2>nul
    if !errorlevel! == 0 (
        echo ✅ Found Visual Studio Build Tools (MSVC compiler available)
        set BUILD_GENERATOR=""
        set CMAKE_EXTRA_FLAGS=-A x64
        set COMPILER_FOUND=1
    )
)

REM 4. Try MinGW-w64 as fallback
if !COMPILER_FOUND! == 0 (
    where /q gcc 2>nul
    if !errorlevel! == 0 (
        where /q mingw32-make 2>nul
        if !errorlevel! == 0 (
            echo ✅ Found MinGW-w64 (GCC compiler)
            set BUILD_GENERATOR="MinGW Makefiles"
            set CMAKE_EXTRA_FLAGS=-DCMAKE_MAKE_PROGRAM=mingw32-make
            set COMPILER_FOUND=1
        ) else (
            where /q make 2>nul
            if !errorlevel! == 0 (
                echo ✅ Found MinGW-w64 (GCC compiler)
                set BUILD_GENERATOR="MinGW Makefiles"
                set CMAKE_EXTRA_FLAGS=""
                set COMPILER_FOUND=1
            )
        )
    )
)

REM Handle case where no compiler was found
if !COMPILER_FOUND! == 0 (
    echo.
    echo ❌ No suitable C++ compiler found!
    echo.
    echo You may have Visual Studio Code, but you need a C++ compiler to build this project.
    echo Visual Studio Code is a code editor, not a compiler.
    echo.
    echo Please install ONE of the following:
    echo.
    echo 1. Visual Studio 2019/2022 Community ^(free^):
    echo    https://visualstudio.microsoft.com/downloads/
    echo    - Install "Desktop development with C++" workload
    echo.
    echo 2. Build Tools for Visual Studio ^(smaller download^):
    echo    https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
    echo    - Install "C++ build tools" workload
    echo.
    echo 3. MinGW-w64 via winget ^(command line^):
    echo    winget install mingw-w64
    echo.
    echo 4. MinGW-w64 via MSYS2 ^(manual^):
    echo    https://www.msys2.org/
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

REM Configure with CMake using detected compiler
if "!BUILD_GENERATOR!" == """" (
    echo Configuring with auto-detected generator...
    cmake .. !CMAKE_EXTRA_FLAGS! -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
) else (
    echo Configuring with !BUILD_GENERATOR!...
    cmake .. -G !BUILD_GENERATOR! !CMAKE_EXTRA_FLAGS! -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
)

if !errorlevel! neq 0 (
    echo.
    echo ❌ CMake configuration failed!
    echo.
    pause
    cd ..
    exit /b 1
)

REM Build the project
echo.
echo Building plugin...
if "!BUILD_GENERATOR!" == """MinGW Makefiles""" (
    REM MinGW uses make instead of cmake --build
    if "!CMAKE_EXTRA_FLAGS!" == "-DCMAKE_MAKE_PROGRAM=mingw32-make" (
        mingw32-make
    ) else (
        make
    )
) else (
    cmake --build . --config %BUILD_TYPE%
)

if !errorlevel! neq 0 (
    echo.
    echo ❌ Build failed!
    echo.
    echo Troubleshooting tips:
    if "!BUILD_GENERATOR!" == """MinGW Makefiles""" (
        echo - Make sure MinGW-w64 is properly installed and in PATH
        echo - Try installing via: winget install mingw-w64
        echo - Or reinstall via MSYS2: https://www.msys2.org/
    ) else (
        echo - Make sure Visual Studio C++ tools are properly installed
        echo - Try running from "Developer Command Prompt for VS"
        echo - Reinstall Visual Studio with "Desktop development with C++" workload
    )
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