#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build/macOS"
ARTIFACTS_DIR="$PROJECT_ROOT/builds/macOS"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "Error: build_macos.sh must be run on macOS." >&2
    exit 1
fi

command -v cmake >/dev/null || { echo "Error: CMake is required." >&2; exit 1; }

if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    CACHED_SOURCE="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$BUILD_DIR/CMakeCache.txt" | head -n 1)"
    if [[ -n "$CACHED_SOURCE" && "$CACHED_SOURCE" != "$PROJECT_ROOT" ]]; then
        echo "Removing stale macOS CMake tree whose source path is $CACHED_SOURCE"
        rm -rf "$BUILD_DIR"
    fi
fi

if [[ "${MIDI_SPLITTER_VERSION_PREPARED:-0}" != "1" ]]; then
    "$SCRIPT_DIR/generate_version.sh"
fi

cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release --parallel
TEST_STATUS=0
ctest --test-dir "$BUILD_DIR" --build-config Release --output-on-failure || TEST_STATUS=$?

VST3_SOURCE="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/VST3/MIDI Splitter.vst3"
AU_SOURCE="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/AU/MIDI Splitter.component"

for plugin in "$VST3_SOURCE" "$AU_SOURCE"; do
    if [[ ! -d "$plugin" ]]; then
        echo "Error: Expected plugin artifact was not built: $plugin" >&2
        exit 1
    fi
    codesign --force --sign - --timestamp=none "$plugin"
done

mkdir -p "$ARTIFACTS_DIR/VST3" "$ARTIFACTS_DIR/AU"
cmake -E rm -rf "$ARTIFACTS_DIR/VST3/MIDI Splitter.vst3"
cmake -E rm -rf "$ARTIFACTS_DIR/AU/MIDI Splitter.component"
cmake -E copy_directory "$VST3_SOURCE" "$ARTIFACTS_DIR/VST3/MIDI Splitter.vst3"
cmake -E copy_directory "$AU_SOURCE" "$ARTIFACTS_DIR/AU/MIDI Splitter.component"

printf 'macOS VST3: %s\nmacOS AU:   %s\n' \
    "$ARTIFACTS_DIR/VST3/MIDI Splitter.vst3" \
    "$ARTIFACTS_DIR/AU/MIDI Splitter.component"

if [[ "$TEST_STATUS" -ne 0 ]]; then
    echo "Error: macOS build completed, but CTest reported failures (exit code $TEST_STATUS)." >&2
    exit "$TEST_STATUS"
fi
