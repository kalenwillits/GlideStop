@echo off
setlocal enabledelayedexpansion

echo ========================================
echo  GlideStop - Complete Build Script
echo ========================================
echo.
echo This script will:
echo 1. Download X-Plane SDK if needed
echo 2. Build the plugin
echo 3. Show installation instructions
echo.
pause

REM Step 1: Setup SDK if needed
if not exist "SDK\CHeaders\XPLM\XPLMPlugin.h" (
    echo Step 1: Setting up X-Plane SDK...
    call setup-sdk.bat
    if !errorlevel! neq 0 (
        echo ❌ SDK setup failed
        pause
        exit /b 1
    )
) else (
    echo ✅ Step 1: X-Plane SDK already available
)

echo.
echo Step 2: Building plugin...
call build.bat

if !errorlevel! neq 0 (
    echo ❌ Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo ✅ BUILD COMPLETE!
echo ========================================
echo.
echo Next steps:
echo 1. Copy the GlideStop folder to: X-Plane 12\Resources\plugins\
echo 2. Restart X-Plane
echo 3. GlideStop automatically controls brakes - no configuration needed
echo 4. Use rudder/stick inputs to control brakes (see README.md for details)
echo.
echo See QUICKSTART.md for detailed usage instructions
echo.
pause

endlocal