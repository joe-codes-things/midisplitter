#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$SCRIPT_DIR/build_windows.sh" --check-only

"$SCRIPT_DIR/generate_version.sh"

MACOS_STATUS=0
WINDOWS_STATUS=0

MIDI_SPLITTER_VERSION_PREPARED=1 "$SCRIPT_DIR/build_macos.sh" || MACOS_STATUS=$?
MIDI_SPLITTER_VERSION_PREPARED=1 "$SCRIPT_DIR/build_windows.sh" || WINDOWS_STATUS=$?

if [[ "$MACOS_STATUS" -ne 0 || "$WINDOWS_STATUS" -ne 0 ]]; then
    echo "macOS build/test exit code: $MACOS_STATUS"
    echo "Windows build/test exit code: $WINDOWS_STATUS"
    exit 1
fi

echo "macOS and Windows VST3 builds completed."
