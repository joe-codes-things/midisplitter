#!/bin/bash
# build_quiet.sh - Wrapper for build.sh that doesn't hang on large output
# Use this script when building via automation/AI tools

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Starting build (output suppressed to avoid timeout issues)..."
echo "This may take a few minutes..."

# Run build, suppress output to avoid massive log files
"$SCRIPT_DIR/build.sh" > /dev/null 2>&1

# Verify it worked by checking for artifacts
if [ -f "$PROJECT_DIR/Source/BuildVersion.h" ]; then
    echo ""
    echo "✓ Build completed successfully!"
    echo ""
    cat "$PROJECT_DIR/Source/BuildVersion.h"
    echo ""
    
    if [ -d "$PROJECT_DIR/build/MIDISplitterPlugin_artefacts/Release/VST3" ]; then
        echo "✓ VST3 plugin built"
    fi
    
    if [ -d "$PROJECT_DIR/build/MIDISplitterPlugin_artefacts/Release/AU" ]; then
        echo "✓ AU plugin built"
    fi
    
    echo ""
else
    echo "✗ Build failed - BuildVersion.h not found"
    exit 1
fi
