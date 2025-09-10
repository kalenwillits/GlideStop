#!/bin/bash

set -e

echo "========================================"
echo " X-Plane SDK Setup Script for TrimGear"
echo "========================================"
echo

# Check if SDK already exists
if [[ -f "SDK/CHeaders/XPLM/XPLMPlugin.h" ]]; then
    echo "✅ X-Plane SDK already installed"
    echo
    read -p "Do you want to reinstall? (y/N): " REINSTALL
    if [[ ! "$REINSTALL" =~ ^[Yy]$ ]]; then
        echo "Setup cancelled."
        exit 0
    fi
    echo "Removing existing SDK..."
    rm -rf SDK
fi

echo "Downloading X-Plane SDK..."
echo

# Create temp directory
mkdir -p temp

# Check if already downloaded
SDK_ZIP="XPSDK411.zip"
if [[ -f "$SDK_ZIP" ]]; then
    echo "✅ SDK zip file already exists: $SDK_ZIP"
elif [[ -f "temp/$SDK_ZIP" ]]; then
    echo "✅ SDK zip file found in temp: temp/$SDK_ZIP"
    cp "temp/$SDK_ZIP" "$SDK_ZIP"
else
    # Try to download using curl or wget
    if command -v curl &> /dev/null; then
        echo "Downloading SDK using curl..."
        curl -L -o "temp/$SDK_ZIP" \
            -H "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36" \
            "https://developer.x-plane.com/wp-content/uploads/2024/10/XPSDK411.zip"
    elif command -v wget &> /dev/null; then
        echo "Downloading SDK using wget..."
        wget -O "temp/$SDK_ZIP" \
            --user-agent="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36" \
            "https://developer.x-plane.com/wp-content/uploads/2024/10/XPSDK411.zip"
    else
        echo "❌ Neither curl nor wget found"
        echo
        echo "Manual download required:"
        echo "1. Go to: https://developer.x-plane.com/sdk/plugin-sdk-downloads/"
        echo "2. Download the latest X-Plane Plugin SDK"
        echo "3. Save it as 'XPSDK411.zip' in this project directory"
        echo "4. Run this script again"
        echo
        exit 1
    fi
    
    cp "temp/$SDK_ZIP" "$SDK_ZIP"
fi

echo
echo "Extracting SDK..."

# Check if unzip is available
if ! command -v unzip &> /dev/null; then
    echo "❌ unzip command not found"
    echo "Please install unzip and try again"
    echo "  Ubuntu/Debian: sudo apt-get install unzip"
    echo "  macOS: unzip should be pre-installed"
    exit 1
fi

# Extract the ZIP file
unzip -q "$SDK_ZIP"

# The SDK usually extracts to a folder like "XPSDK411" - we need to rename it to "SDK"
for dir in XPSDK*/; do
    if [[ -f "${dir}CHeaders/XPLM/XPLMPlugin.h" ]]; then
        echo "Moving SDK from $dir to SDK..."
        [[ -d "SDK" ]] && rm -rf SDK
        mv "$dir" SDK
        break
    fi
done

# Clean up
rm -rf temp 2>/dev/null || true

# Verify installation
if [[ -f "SDK/CHeaders/XPLM/XPLMPlugin.h" ]]; then
    echo
    echo "========================================"
    echo "✅ X-Plane SDK Setup Complete!"
    echo "========================================"
    echo
    echo "SDK installed in: SDK/"
    echo
    echo "You can now run ./build.sh to compile the TrimGear plugin"
    echo
else
    echo "❌ SDK installation verification failed"
    echo "Expected file: SDK/CHeaders/XPLM/XPLMPlugin.h"
    exit 1
fi