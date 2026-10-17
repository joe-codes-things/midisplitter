#!/bin/bash

# MIDI Splitter Plugin Build Script
# This script builds both VST3 and AU versions of the plugin

set -e  # Exit on error

echo "========================================="
echo "MIDI Splitter Plugin Build Script v0.0.5"
echo "========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get script directory and project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Generate version header with auto-incrementing build number
echo "Generating version info..."
"$SCRIPT_DIR/generate_version.sh"
echo ""

# Build directory
BUILD_DIR="$PROJECT_ROOT/build"

# Check if JUCE exists
if [ ! -d "JUCE" ]; then
    echo -e "${RED}Error: JUCE directory not found!${NC}"
    echo "Please ensure JUCE is installed in the project directory."
    exit 1
fi

echo -e "${GREEN}✓${NC} JUCE found"

# Create build directory
echo ""
echo "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo ""
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} CMake configuration successful"

# Build the project
echo ""
echo "Building plugin..."
echo "This may take a few minutes..."
cmake --build . --config Release

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓${NC} Build successful"

# Code sign the plugins (macOS only)
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo ""
    echo "Code signing plugins..."
    
    # Get version from CMakeLists.txt for plugin names
    VERSION=$(grep "project(MIDISplitterPlugin VERSION" "$SCRIPT_DIR/CMakeLists.txt" | sed 's/.*VERSION \([0-9.]*\).*/\1/')
    VST3_PATH="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/VST3/MIDI Splitter.vst3"
    AU_PATH="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/AU/MIDI Splitter.component"
    
    if [ -d "$VST3_PATH" ]; then
        codesign --force --sign - --timestamp=none "$VST3_PATH" 2>&1 | grep -v "replacing existing signature" || true
        # Remove quarantine if present
        xattr -d com.apple.quarantine "$VST3_PATH" 2>/dev/null || true
        echo -e "${GREEN}✓${NC} VST3 signed"
    fi
    
    if [ -d "$AU_PATH" ]; then
        codesign --force --sign - --timestamp=none "$AU_PATH" 2>&1 | grep -v "replacing existing signature" || true
        # Remove quarantine if present
        xattr -d com.apple.quarantine "$AU_PATH" 2>/dev/null || true
        echo -e "${GREEN}✓${NC} AU signed"
    fi
fi

# Find built plugins
echo ""
echo "========================================="
echo "Build Results:"
echo "========================================="

# Get version from CMakeLists.txt for plugin names
VERSION=$(grep "project(MIDISplitterPlugin VERSION" "$PROJECT_ROOT/CMakeLists.txt" | sed 's/.*VERSION \([0-9.]*\).*/\1/')

# Check for VST3 (built with original product name)
VST3_PATH="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/VST3/MIDI Splitter.vst3"
if [ -d "$VST3_PATH" ]; then
    echo -e "${GREEN}✓${NC} VST3 plugin built successfully"
    echo "   Location: $VST3_PATH"
    VST3_SIZE=$(du -sh "$VST3_PATH" | cut -f1)
    echo "   Size: $VST3_SIZE"
else
    echo -e "${YELLOW}⚠${NC} VST3 plugin not found"
fi

# Check for AU (macOS only) (built with original product name)
if [[ "$OSTYPE" == "darwin"* ]]; then
    AU_PATH="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/AU/MIDI Splitter.component"
    if [ -d "$AU_PATH" ]; then
        echo -e "${GREEN}✓${NC} AU plugin built successfully"
        echo "   Location: $AU_PATH"
        AU_SIZE=$(du -sh "$AU_PATH" | cut -f1)
        echo "   Size: $AU_SIZE"
    else
        echo -e "${YELLOW}⚠${NC} AU plugin not found"
    fi
fi

# Create convenience aliases
echo ""
echo "========================================="
echo "Build Complete!"
echo "========================================="
echo ""
echo "Quick access scripts available in Scripts/ folder:"
echo "  ./Scripts/goto-builds.sh  - View all builds"
echo "  ./Scripts/goto-vst3.sh    - Go to VST3 build"
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "  ./Scripts/goto-au.sh      - Go to AU build"
fi
echo ""

echo ""
echo "========================================="
echo "Build Complete!"
echo "========================================="
echo ""

# Install plugins on macOS when ready.
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "To install plugins to the system folders (administrator approval required), run:"
    echo "  ./Scripts/sign_and_install.sh"
else
    echo "To install plugins:"
    echo "  Run: ./Scripts/sign_and_install.sh"
    echo ""
    echo "Or manually copy:"
    if [ -d "$VST3_PATH" ]; then
        echo "  VST3: Copy to ~/Library/Audio/Plug-Ins/VST3/ (macOS)"
        echo "        or C:\\Program Files\\Common Files\\VST3\\ (Windows)"
    fi
fi

echo ""
echo "To navigate to build directories, use:"
echo "  ./goto-builds.sh  - View all builds"
if [ -f "$SCRIPT_DIR/goto-vst3.sh" ]; then
    echo "  ./goto-vst3.sh    - Go to VST3 build"
fi
if [ -f "$SCRIPT_DIR/goto-au.sh" ]; then
    echo "  ./goto-au.sh      - Go to AU build"
fi
echo ""

# Create versioned exports with NLA naming format
echo "========================================="
echo "Creating Versioned Exports..."
echo "========================================="

# Get version from CMakeLists.txt
VERSION=$(grep "project(MIDISplitterPlugin VERSION" "$SCRIPT_DIR/CMakeLists.txt" | sed 's/.*VERSION \([0-9.]*\).*/\1/')
BUILD_COUNT=$(cat "$SCRIPT_DIR/.build_count" 2>/dev/null || echo "0")

# Create exports directory
EXPORTS_DIR="$SCRIPT_DIR/exports"
mkdir -p "$EXPORTS_DIR"

# Export VST3 with NLA naming format (no version number)
if [ -d "$VST3_PATH" ]; then
    EXPORT_NAME="nla_midisplitter.vst3"
    EXPORT_PATH="$EXPORTS_DIR/$EXPORT_NAME"
    cp -R "$VST3_PATH" "$EXPORT_PATH"
    echo -e "${GREEN}✓${NC} Exported VST3: $EXPORT_NAME"
fi

# Export AU with NLA naming format (macOS only, no version number)
if [[ "$OSTYPE" == "darwin"* ]] && [ -d "$AU_PATH" ]; then
    EXPORT_NAME="nla_midisplitter.component"
    EXPORT_PATH="$EXPORTS_DIR/$EXPORT_NAME"
    cp -R "$AU_PATH" "$EXPORT_PATH"
    echo -e "${GREEN}✓${NC} Exported AU: $EXPORT_NAME"
fi

echo ""
echo "Exports saved to: $EXPORTS_DIR"
echo ""
