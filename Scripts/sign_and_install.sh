#!/bin/bash
# sign_and_install.sh - Properly sign and install plugins for macOS development

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
VST3_SRC="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/VST3/MIDI Splitter.vst3"
AU_SRC="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/AU/MIDI Splitter.component"

# Auto-derive version from CMakeLists.txt
BASE_VERSION=$(grep "project(MIDISplitterPlugin VERSION" "$PROJECT_DIR/CMakeLists.txt" | sed 's/.*VERSION \([0-9.]*\).*/\1/')

# Use NLA naming format: nla_midisplitter_v0.0.13
VST3_DEST="/Library/Audio/Plug-Ins/VST3/nla_midisplitter_v${BASE_VERSION}.vst3"
AU_DEST="/Library/Audio/Plug-Ins/Components/nla_midisplitter_v${BASE_VERSION}.component"

echo "========================================="
echo "MIDI Splitter Plugin Sign & Install"
echo "========================================="

# Check if plugins exist
if [ ! -d "$VST3_SRC" ]; then
    echo "Error: VST3 plugin not found at $VST3_SRC"
    echo "Please build the plugin first with ./build.sh"
    exit 1
fi

if [ ! -d "$AU_SRC" ]; then
    echo "Error: AU plugin not found at $AU_SRC"
    echo "Please build the plugin first with ./build.sh"
    exit 1
fi

# Remove old plugins if they exist
echo "Removing old plugins..."
sudo rm -rf "$VST3_DEST"
sudo rm -rf "$AU_DEST"

# Sign the built plugins
echo ""
echo "Code signing plugins..."
echo "Signing VST3..."
codesign --force --sign - --timestamp=none "$VST3_SRC" 2>&1 | grep -v "replacing existing signature" || true
echo "Signing AU..."
codesign --force --sign - --timestamp=none "$AU_SRC" 2>&1 | grep -v "replacing existing signature" || true

# Remove quarantine attributes (if any)
echo ""
echo "Removing quarantine attributes..."
xattr -d com.apple.quarantine "$VST3_SRC" 2>/dev/null || true
xattr -d com.apple.quarantine "$AU_SRC" 2>/dev/null || true

# Copy to plugin folders
echo ""
echo "Installing plugins..."
sudo cp -R "$VST3_SRC" "$VST3_DEST"
sudo cp -R "$AU_SRC" "$AU_DEST"

# Sign again after copy (copy can invalidate signature)
echo ""
echo "Re-signing installed plugins..."
sudo codesign --force --sign - --timestamp=none "$VST3_DEST" 2>&1 | grep -v "replacing existing signature" || true
sudo codesign --force --sign - --timestamp=none "$AU_DEST" 2>&1 | grep -v "replacing existing signature" || true

# Remove quarantine from installed copies
xattr -d com.apple.quarantine "$VST3_DEST" 2>/dev/null || true
xattr -d com.apple.quarantine "$AU_DEST" 2>/dev/null || true

# Verify signatures
echo ""
echo "Verifying signatures..."
echo "VST3:"
sudo codesign --verify --verbose "$VST3_DEST" 2>&1 | head -2
echo "AU:"
sudo codesign --verify --verbose "$AU_DEST" 2>&1 | head -2

echo ""
echo "========================================="
echo "Installation complete!"
echo "========================================="
echo "VST3: $VST3_DEST"
echo "AU: $AU_DEST"
echo ""
echo "Plugins installed to SYSTEM folders (/Library/Audio/Plug-Ins/)"
echo "These are accessible to all users on this Mac."
echo ""
echo "Next steps:"
echo "1. Restart your DAW"
echo "2. If Logic doesn't show the AU: auvaltool -v aumf MiAr Mdar"
echo "3. If Cubase crashes: Check Console.app for crash logs"
echo "4. For IPC debugging: Check ~/Documents/MIDI Splitter Logs/"
