#!/bin/bash
# Quick navigation to main build output directory
cd "$(dirname "$0")/../build/MIDISplitterPlugin_artefacts/Release"
pwd
echo ""
echo "Available builds:"
ls -d */ 2>/dev/null || echo "No builds found"
