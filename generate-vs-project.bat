@echo off
setlocal enabledelayedexpansion

echo ========================================
echo  Generate Visual Studio Project Files
echo ========================================
echo.

REM Check if we're in the right directory
if not exist "src\glidestop.cpp" (
    echo ERROR: This script must be run from the project root directory
    pause
    exit /b 1
)

REM Check for X-Plane SDK
if not exist "SDK\CHeaders\XPLM\XPLMPlugin.h" (
    echo ❌ X-Plane SDK not found!
    echo Please run setup-sdk.bat first
    pause
    exit /b 1
)

REM Create build directory for VS projects
if not exist "vs-build" mkdir vs-build

REM Detect Visual Studio version
set VS_GENERATOR=""
set VS_YEAR=""

REM Try VS 2022 first
where /q devenv 2>nul
if !errorlevel! == 0 (
    for /f "tokens=*" %%i in ('where devenv 2^>nul') do (
        if "!VS_GENERATOR!" == """" (
            echo %%i | findstr /i "2022" >nul
            if !errorlevel! == 0 (
                set VS_GENERATOR="Visual Studio 17 2022"
                set VS_YEAR="2022"
                echo ✅ Found Visual Studio 2022
            )
        )
    )
)

REM Try VS 2019 if 2022 not found
if !VS_GENERATOR! == """" (
    for /f "tokens=*" %%i in ('where devenv 2^>nul') do (
        echo %%i | findstr /i "2019" >nul
        if !errorlevel! == 0 (
            set VS_GENERATOR="Visual Studio 16 2019"
            set VS_YEAR="2019"
            echo ✅ Found Visual Studio 2019
        )
    )
)

if !VS_GENERATOR! == """" (
    echo ❌ Visual Studio not found!
    echo Please install Visual Studio 2019 or 2022 with C++ development tools
    pause
    exit /b 1
)

echo.
echo Generating Visual Studio project files...
echo.

cd vs-build

REM Generate VS project files
cmake .. -G !VS_GENERATOR! -A x64

if !errorlevel! neq 0 (
    echo.
    echo ❌ Project generation failed!
    pause
    cd ..
    exit /b 1
)

cd ..

echo.
echo ========================================
echo ✅ Visual Studio Project Generated!
echo ========================================
echo.
echo Project files created in: vs-build\
echo.
echo To build in Visual Studio:
echo 1. Open: vs-build\GlideStop.sln
echo 2. Set build configuration to "Release"
echo 3. Build → Build Solution (Ctrl+Shift+B)
echo.
echo OR build from command line:
echo    cmake --build vs-build --config Release
echo.

set /p "OPEN_VS=Open Visual Studio now? (Y/n): "
if /i not "!OPEN_VS!" == "n" (
    if exist "vs-build\GlideStop.sln" (
        echo Opening Visual Studio...
        start "" "vs-build\GlideStop.sln"
    )
)

endlocal