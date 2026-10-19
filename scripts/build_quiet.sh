#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
source "$SCRIPT_DIR/branch_config.sh"
GIT_BRANCH="$(midisplitter_branch_name "$PROJECT_ROOT")"

echo "Building and testing the macOS plugins..."
"$SCRIPT_DIR/build_macos.sh" > /dev/null
echo "macOS VST3 and AU builds completed in $(midisplitter_export_dir "$PROJECT_ROOT" "$GIT_BRANCH")/macOS/ ($(midisplitter_product_name "$GIT_BRANCH"))"
