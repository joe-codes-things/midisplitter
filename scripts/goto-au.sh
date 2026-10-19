#!/bin/bash
# Quick navigation to the staged AU artifact
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
source "$SCRIPT_DIR/branch_config.sh"
BRANCH="$(midisplitter_branch_name "$PROJECT_ROOT")"
cd "$(midisplitter_export_dir "$PROJECT_ROOT" "$BRANCH")/macOS/AU"
pwd
ls -lh
