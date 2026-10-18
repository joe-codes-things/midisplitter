#!/bin/bash
# Quick navigation to staged macOS build artifacts
cd "$(dirname "$0")/../builds/macOS"
pwd
echo ""
echo "Available builds:"
ls -d */ 2>/dev/null || echo "No builds found"
