@echo off
setlocal enabledelayedexpansion

echo ========================================
echo  X-Plane SDK Setup Script for TrimGear
echo ========================================
echo.

REM Check if SDK already exists
if exist "SDK\CHeaders\XPLM\XPLMPlugin.h" (
    echo ✅ X-Plane SDK already installed
    echo.
    set /p "REINSTALL=Do you want to reinstall? (y/N): "
    if /i not "!REINSTALL!" == "y" (
        echo Setup cancelled.
        pause
        exit /b 0
    )
    echo Removing existing SDK...
    rmdir /s /q SDK 2>nul
)

echo Downloading X-Plane SDK...
echo.

REM Create temp directory
if not exist "temp" mkdir temp

REM Check if already downloaded
set SDK_ZIP=XPSDK411.zip
if exist "!SDK_ZIP!" (
    echo ✅ SDK zip file already exists: !SDK_ZIP!
    goto extract
) else if exist "temp\!SDK_ZIP!" (
    echo ✅ SDK zip file found in temp: temp\!SDK_ZIP!
    copy "temp\!SDK_ZIP!" "!SDK_ZIP!" >nul
    goto extract
)

REM Try to download using PowerShell (Windows 10+)
echo Downloading SDK using PowerShell...
powershell -Command "try { Invoke-WebRequest -Uri 'https://developer.x-plane.com/wp-content/uploads/2024/10/XPSDK411.zip' -OutFile 'temp\%SDK_ZIP%' -UserAgent 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36' } catch { exit 1 }"

if !errorlevel! neq 0 (
    echo ❌ Download failed with PowerShell
    echo.
    echo Manual download required:
    echo 1. Go to: https://developer.x-plane.com/sdk/plugin-sdk-downloads/
    echo 2. Download the latest X-Plane Plugin SDK
    echo 3. Save it as 'XPSDK411.zip' in this project directory
    echo 4. Run this script again
    echo.
    pause
    exit /b 1
)

copy "temp\!SDK_ZIP!" "!SDK_ZIP!" >nul

:extract
echo.
echo Extracting SDK...

REM Extract using PowerShell
powershell -Command "Expand-Archive -Path '%SDK_ZIP%' -DestinationPath '.' -Force"

if !errorlevel! neq 0 (
    echo ❌ Extraction failed
    echo Please extract !SDK_ZIP! manually to the project directory
    pause
    exit /b 1
)

REM The SDK usually extracts to a folder like "XPSDK411" - we need to rename it to "SDK"
for /d %%d in (XPSDK*) do (
    if exist "%%d\CHeaders\XPLM\XPLMPlugin.h" (
        echo Moving SDK from %%d to SDK...
        if exist "SDK" rmdir /s /q SDK
        move "%%d" SDK >nul
        goto :sdk_moved
    )
)

echo ❌ Could not find SDK in extracted files
dir /b
pause
exit /b 1

:sdk_moved
REM Clean up
if exist "temp" rmdir /s /q temp 2>nul

REM Verify installation
if exist "SDK\CHeaders\XPLM\XPLMPlugin.h" (
    echo.
    echo ========================================
    echo ✅ X-Plane SDK Setup Complete!
    echo ========================================
    echo.
    echo SDK installed in: SDK\
    echo.
    echo You can now run build.bat to compile the TrimGear plugin
    echo.
) else (
    echo ❌ SDK installation verification failed
    echo Expected file: SDK\CHeaders\XPLM\XPLMPlugin.h
)

pause
endlocal