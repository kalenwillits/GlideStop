#!/bin/bash

set -e  # Exit on any error

echo "========================================"
echo " GlideStop X-Plane Plugin Build Script"
echo "========================================"
echo


# Check if we're in the right directory
if [[ ! -f "src/glidestop.cpp" ]]; then
    echo "❌ ERROR: This script must be run from the project root directory"
    echo "Current directory: $(pwd)"
    exit 1
fi

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    PLUGIN_NAME="mac.xpl"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
    PLUGIN_NAME="lin.xpl"
else
    echo "❌ Unsupported platform: $OSTYPE"
    exit 1
fi

echo "Building for: $PLATFORM"

# Create build directory
mkdir -p build

# Check for X-Plane SDK
SDK_FOUND=0
if [[ -f "SDK/CHeaders/XPLM/XPLMPlugin.h" ]]; then
    echo "✅ X-Plane SDK found in SDK directory"
    SDK_FOUND=1
elif [[ -n "$XPLANE_SDK_PATH" && -f "$XPLANE_SDK_PATH/CHeaders/XPLM/XPLMPlugin.h" ]]; then
    echo "✅ X-Plane SDK found at XPLANE_SDK_PATH: $XPLANE_SDK_PATH"
    SDK_FOUND=1
fi

if [[ $SDK_FOUND -eq 0 ]]; then
    echo
    echo "❌ X-Plane SDK not found!"
    echo
    echo "Please do one of the following:"
    echo "  1. Download SDK from https://developer.x-plane.com/sdk/plugin-sdk-downloads/"
    echo "  2. Extract it to the 'SDK' folder in this project directory"
    echo "  3. OR set XPLANE_SDK_PATH environment variable to SDK location"
    echo
    echo "Expected file: SDK/CHeaders/XPLM/XPLMPlugin.h"
    echo
    exit 1
fi

# Check for required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo "❌ ERROR: $1 is required but not installed"
        echo "Please install $1 and try again"
        exit 1
    fi
}

check_tool cmake
if [[ "$PLATFORM" == "macOS" ]]; then
    # Check for Xcode command line tools
    if ! xcode-select -p &> /dev/null; then
        echo "❌ ERROR: Xcode command line tools not installed"
        echo "Please run: xcode-select --install"
        exit 1
    fi
    echo "✅ Xcode command line tools found"
else
    # Linux - check for GCC or Clang
    if ! (command -v gcc &> /dev/null || command -v clang &> /dev/null); then
        echo "❌ ERROR: No C++ compiler found (gcc or clang)"
        echo "Please install build tools:"
        echo "  Ubuntu/Debian: sudo apt-get install build-essential"
        echo "  CentOS/RHEL: sudo yum groupinstall \"Development Tools\""
        exit 1
    fi
    if command -v gcc &> /dev/null; then
        echo "✅ GCC compiler found"
    elif command -v clang &> /dev/null; then
        echo "✅ Clang compiler found"
    fi
fi

echo
echo "Building GlideStop Plugin..."
echo

cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo
echo "Building plugin..."
if [[ "$PLATFORM" == "macOS" ]]; then
    make -j$(sysctl -n hw.ncpu)
else
    make -j$(nproc)
fi

# Create the plugin directory structure
echo
echo "Creating plugin directory structure..."
make plugin

cd ..

# Check if build was successful
PLUGIN_PATH="build/GlideStop/$PLUGIN_NAME"
if [[ -f "$PLUGIN_PATH" ]]; then
    echo
    echo "========================================"
    echo "✅ BUILD SUCCESSFUL!"
    echo "========================================"
    echo
    echo "Plugin built: $PLUGIN_PATH"
    echo
    echo "Installation:"
    echo "1. Copy the entire 'GlideStop' folder to:"
    echo "   X-Plane 12/Resources/plugins/"
    echo
    echo "2. Final path should be:"
    echo "   X-Plane 12/Resources/plugins/GlideStop/$PLUGIN_NAME"
    echo
    echo "The plugin is now ready for installation!"
    
    # Show file info
    echo "Plugin file details:"
    ls -la "$PLUGIN_PATH"
    
    # Show the complete directory structure
    echo
    echo "Plugin directory structure:"
    ls -la build/GlideStop/
    
    if [[ "$PLATFORM" == "macOS" ]]; then
        echo
        echo "Opening build directory..."
        open "build/GlideStop"
    fi
else
    echo
    echo "❌ Build completed but plugin file not found!"
    echo "Expected: $PLUGIN_PATH"
    exit 1
fi