#!/bin/bash

set -e

echo "========================================"
echo " GlideStop - Complete Build Script"
echo "========================================"
echo
echo "This script will:"
echo "1. Download X-Plane SDK if needed"
echo "2. Build the plugin"
echo "3. Show installation instructions"
echo
read -p "Press Enter to continue..."

# Step 1: Setup SDK if needed
if [[ ! -f "SDK/CHeaders/XPLM/XPLMPlugin.h" ]]; then
    echo "Step 1: Setting up X-Plane SDK..."
    if [[ -f "setup-sdk.sh" ]]; then
        ./setup-sdk.sh
    else
        echo "⚠️  setup-sdk.sh not found. Please manually download the X-Plane SDK:"
        echo "   1. Download from: https://developer.x-plane.com/sdk/plugin-sdk-downloads/"
        echo "   2. Extract to 'SDK' folder in this directory"
        echo "   3. Re-run this script"
        exit 1
    fi
else
    echo "✅ Step 1: X-Plane SDK already available"
fi

echo
echo "Step 2: Building plugin..."
./build.sh release

echo
echo "========================================"
echo "✅ BUILD COMPLETE!"
echo "========================================"
echo
echo "Next steps:"
echo "1. Copy the GlideStop folder to: X-Plane 12/Resources/plugins/"
echo "2. Restart X-Plane"
echo "3. GlideStop automatically controls brakes - no joystick configuration needed"
echo "   Controls brakes automatically based on rudder and stick inputs"
echo "4. Configure wake category and enable in Plugins → GlideStop menu"
echo
echo "See README.md for detailed usage instructions"
echo