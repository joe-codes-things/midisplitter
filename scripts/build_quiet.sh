#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Building and testing the macOS plugins..."
"$SCRIPT_DIR/build_macos.sh" > /dev/null
echo "macOS VST3 and AU builds completed in $PROJECT_ROOT/exports/production-builds/macOS/"
