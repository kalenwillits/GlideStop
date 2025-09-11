#!/bin/bash

# GlideStop Plugin Verification Script
# This script helps verify that the plugin is built correctly and ready for X-Plane

echo "======================================="
echo " GlideStop Plugin Verification"
echo "======================================="
echo

# Check if we're in the right directory
if [[ ! -f "src/glidestop.cpp" ]]; then
    echo "❌ ERROR: Run this script from the project root directory"
    exit 1
fi

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLUGIN_NAME="mac.xpl"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLUGIN_NAME="lin.xpl"
else
    echo "❌ Unsupported platform: $OSTYPE"
    exit 1
fi

PLUGIN_PATH="build/GlideStop/$PLUGIN_NAME"

echo "Platform: $(uname -s)"
echo "Expected plugin: $PLUGIN_PATH"
echo


# Check if plugin exists
if [[ ! -f "$PLUGIN_PATH" ]]; then
    echo "❌ Plugin not found!"
    echo "   Run './build.sh' first to build the plugin"
    exit 1
fi

echo "✅ Plugin file exists: $PLUGIN_PATH"

# Check file properties
echo "📋 Plugin file details:"
ls -la "$PLUGIN_PATH"
echo

# Check if it's a valid shared library
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if file "$PLUGIN_PATH" | grep -q "shared object"; then
        echo "✅ Plugin is a valid shared library"
    else
        echo "❌ Plugin is not a valid shared library"
        exit 1
    fi
    
    # Check for X-Plane entry points
    if nm -D "$PLUGIN_PATH" 2>/dev/null | grep -q "XPluginStart"; then
        echo "✅ Plugin has X-Plane entry points"
        echo "   Found entry points:"
        nm -D "$PLUGIN_PATH" 2>/dev/null | grep "XPlugin" | sed 's/^/   /'
    else
        echo "❌ Plugin missing X-Plane entry points"
        exit 1
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    if file "$PLUGIN_PATH" | grep -q "bundle"; then
        echo "✅ Plugin is a valid macOS bundle"
    else
        echo "❌ Plugin is not a valid macOS bundle"
        exit 1
    fi
fi

echo
echo "📁 Plugin directory structure:"
ls -la build/GlideStop/
echo

echo "======================================="
echo "✅ PLUGIN VERIFICATION SUCCESSFUL!"
echo "======================================="
echo
echo "Installation instructions:"
echo "1. Copy the entire 'build/GlideStop' folder to:"
echo "   X-Plane 12/Resources/plugins/"
echo
echo "2. The final path should be:"
echo "   X-Plane 12/Resources/plugins/GlideStop/$PLUGIN_NAME"
echo
echo "3. Restart X-Plane and look for 'GlideStop' in the Plugins menu"
echo