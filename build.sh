#!/bin/bash

# TrimGear Plugin Build Script
# 
# This script builds the TrimGear plugin for X-Plane 12
# Usage: ./build.sh [clean|release|package]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}GlideStop Plugin Build Script${NC}"
echo "=============================="

# Handle command line arguments
case "${1:-}" in
    clean)
        echo "Cleaning build directory..."
        rm -rf build
        echo -e "${GREEN}✅ Clean complete${NC}"
        exit 0
        ;;
    release)
        BUILD_TYPE="Release"
        PACKAGE=true
        ;;
    package)
        PACKAGE=true
        BUILD_TYPE="Release"
        ;;
    *)
        BUILD_TYPE="Debug"
        ;;
esac

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

# Enter build directory
cd build

# Configure with CMake
echo "Configuring with CMake (${BUILD_TYPE})..."
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..

# Build the plugin
echo "Building GlideStop plugin..."
cmake --build .

# Create plugin directory structure
echo "Creating plugin package..."
cmake --build . --target plugin

# Create distributable package if requested
if [ "${PACKAGE:-}" = "true" ]; then
    echo "Creating distributable package..."
    cmake --build . --target package
    echo -e "${GREEN}✅ Package created in build/package/TrimGear/${NC}"
fi

echo ""
echo -e "${GREEN}✅ Build complete!${NC}"
echo -e "Plugin location: ${YELLOW}build/GlideStop/lin.xpl${NC}"
echo ""
echo "To install:"
echo "1. Copy the entire 'GlideStop' folder to X-Plane/Resources/plugins/"
echo "2. Restart X-Plane"
echo "3. Look for 'GlideStop' in the Plugins menu"