#!/bin/bash
# generate_version.sh - Auto-generates version header with build number

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SOURCE_DIR="$PROJECT_DIR/Source"
VERSION_FILE="$PROJECT_DIR/.build_count"

# Get base version from CMakeLists.txt
BASE_VERSION=$(grep "project(MIDISplitterPlugin VERSION" "$PROJECT_DIR/CMakeLists.txt" | sed 's/.*VERSION \([0-9.]*\).*/\1/')

# Get or initialize build count (for tracking only, not used in version string)
if [ -f "$VERSION_FILE" ]; then
    BUILD_COUNT=$(cat "$VERSION_FILE")
else
    BUILD_COUNT=0
fi

# Increment build count
BUILD_COUNT=$((BUILD_COUNT + 1))
echo "$BUILD_COUNT" > "$VERSION_FILE"

# Generate build timestamp
BUILD_DATE=$(date +"%Y%m%d")
BUILD_TIME=$(date +"%H%M%S")

# Version string is just the base version (3 digits: 0.0.10)
FULL_VERSION="${BASE_VERSION}"

# Generate version header
cat > "$SOURCE_DIR/BuildVersion.h" << EOF
#pragma once

// Auto-generated version header - DO NOT EDIT
// Generated: $(date)

#define PLUGIN_BASE_VERSION "${BASE_VERSION}"
#define PLUGIN_BUILD_NUMBER ${BUILD_COUNT}
#define PLUGIN_FULL_VERSION "${FULL_VERSION}"
#define PLUGIN_BUILD_DATE "${BUILD_DATE}"
#define PLUGIN_BUILD_TIME "${BUILD_TIME}"
#define PLUGIN_BUILD_TIMESTAMP "${BUILD_DATE}_${BUILD_TIME}"

// For display in UI
#define PLUGIN_DISPLAY_VERSION "MIDI Splitter v" PLUGIN_FULL_VERSION
EOF

echo "Generated version: $FULL_VERSION (build #$BUILD_COUNT)"
