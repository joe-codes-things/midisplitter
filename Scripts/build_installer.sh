#!/bin/bash
# build_installer.sh
# Builds a macOS .pkg installer for MIDI Splitter with optional VST3, AU, and PDF guide components.
# Requires: pkgbuild, productbuild (both included with Xcode Command Line Tools)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
EXPORTS_DIR="$PROJECT_DIR/exports"
INSTALLER_DIR="$PROJECT_DIR/installer"
WORK_DIR="$PROJECT_DIR/installer_work"  # scratch space, cleaned up after

# ── Resolve version ──────────────────────────────────────────────────────────
VERSION=$(grep "project(MIDISplitterPlugin VERSION" "$PROJECT_DIR/CMakeLists.txt" \
          | sed 's/.*VERSION \([0-9.]*\).*/\1/')
INSTALLER_NAME="MIDISplitter_v${VERSION}_macOS"

# ── Source plugin paths ──────────────────────────────────────────────────────
VST3_SRC="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/VST3/MIDI Splitter.vst3"
AU_SRC="$BUILD_DIR/MIDISplitterPlugin_artefacts/Release/AU/MIDI Splitter.component"
GUIDE_SRC="$PROJECT_DIR/Resources/MIDI_Splitter_User_Guide.pdf"

# ── Destination paths (inside the installer payload) ────────────────────────
VST3_DEST_DIR="/Library/Audio/Plug-Ins/VST3"
AU_DEST_DIR="/Library/Audio/Plug-Ins/Components"
GUIDE_DEST_DIR_RELATIVE="Documents/Nine Lives Audio"  # relative to ~, installed via postinstall

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================="
echo "MIDI Splitter Installer Builder"
echo "Version: $VERSION"
echo "========================================="
echo ""

# ── Preflight checks ─────────────────────────────────────────────────────────
if [ ! -d "$VST3_SRC" ]; then
    echo -e "${RED}Error: VST3 plugin not found at:${NC}"
    echo "  $VST3_SRC"
    echo "Run ./build.sh first."
    exit 1
fi

if [ ! -d "$AU_SRC" ]; then
    echo -e "${RED}Error: AU plugin not found at:${NC}"
    echo "  $AU_SRC"
    echo "Run ./build.sh first."
    exit 1
fi

GUIDE_AVAILABLE=true
if [ ! -f "$GUIDE_SRC" ]; then
    echo -e "${YELLOW}Note: PDF guide not found at Resources/MIDI_Splitter_User_Guide.pdf${NC}"
    echo "      Guide component will be built as a placeholder."
    GUIDE_AVAILABLE=false
fi

# ── Clean and create work directory ─────────────────────────────────────────
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR/vst3_payload/$VST3_DEST_DIR"
mkdir -p "$WORK_DIR/au_payload/$AU_DEST_DIR"
mkdir -p "$WORK_DIR/guide_payload/tmp_guide"
mkdir -p "$WORK_DIR/pkgs"
mkdir -p "$EXPORTS_DIR"

# ── Sign plugins (ad-hoc) ────────────────────────────────────────────────────
echo "Signing plugins..."
codesign --force --sign - --timestamp=none "$VST3_SRC" 2>&1 | grep -v "replacing existing signature" || true
codesign --force --sign - --timestamp=none "$AU_SRC"   2>&1 | grep -v "replacing existing signature" || true
echo -e "${GREEN}Signed${NC}"
echo ""

# ── Copy payloads ────────────────────────────────────────────────────────────
echo "Staging payloads..."

# Copy VST3 without version number in filename
cp -R "$VST3_SRC" "$WORK_DIR/vst3_payload/$VST3_DEST_DIR/nla_midisplitter.vst3"

# Copy AU without version number in filename
cp -R "$AU_SRC" "$WORK_DIR/au_payload/$AU_DEST_DIR/nla_midisplitter.component"

if [ "$GUIDE_AVAILABLE" = true ]; then
    cp "$GUIDE_SRC" "$WORK_DIR/guide_payload/tmp_guide/MIDI_Splitter_User_Guide.pdf"
else
    # Placeholder so the component pkg is still valid
    echo "PDF guide will be available in a future release." \
        > "$WORK_DIR/guide_payload/tmp_guide/MIDI_Splitter_User_Guide_placeholder.txt"
fi
echo -e "${GREEN}Done${NC}"
echo ""

# ── Build component packages ─────────────────────────────────────────────────
echo "Building component packages..."

# VST3
pkgbuild \
    --root "$WORK_DIR/vst3_payload" \
    --identifier "com.ninelivesaudio.midisplitter.vst3" \
    --version "$VERSION" \
    --install-location "/" \
    "$WORK_DIR/pkgs/MIDISplitter_VST3.pkg"
echo -e "${GREEN}VST3 component pkg built${NC}"

# AU
pkgbuild \
    --root "$WORK_DIR/au_payload" \
    --identifier "com.ninelivesaudio.midisplitter.au" \
    --version "$VERSION" \
    --install-location "/" \
    "$WORK_DIR/pkgs/MIDISplitter_AU.pkg"
echo -e "${GREEN}AU component pkg built${NC}"

# Guide — installs to a temp location; postinstall script moves it to ~/Documents/Nine Lives Audio/
# We use a postinstall script so it lands in the current user's home folder.
mkdir -p "$WORK_DIR/guide_scripts"
cat > "$WORK_DIR/guide_scripts/postinstall" << 'POSTINSTALL'
#!/bin/bash
# Move guide from temp install location to the current user's Documents folder
DEST="$HOME/Documents/Nine Lives Audio"
mkdir -p "$DEST"
SRC="/tmp/nla_guide_staging"
if [ -d "$SRC" ]; then
    cp -R "$SRC/"* "$DEST/" 2>/dev/null || true
    rm -rf "$SRC"
fi
exit 0
POSTINSTALL
chmod +x "$WORK_DIR/guide_scripts/postinstall"

# Adjust guide payload to stage into /tmp
rm -rf "$WORK_DIR/guide_payload_pkg"
mkdir -p "$WORK_DIR/guide_payload_pkg/tmp/nla_guide_staging"
if [ "$GUIDE_AVAILABLE" = true ]; then
    cp "$GUIDE_SRC" "$WORK_DIR/guide_payload_pkg/tmp/nla_guide_staging/"
else
    cp "$WORK_DIR/guide_payload/tmp_guide/"* "$WORK_DIR/guide_payload_pkg/tmp/nla_guide_staging/"
fi

pkgbuild \
    --root "$WORK_DIR/guide_payload_pkg" \
    --scripts "$WORK_DIR/guide_scripts" \
    --identifier "com.ninelivesaudio.midisplitter.guide" \
    --version "$VERSION" \
    --install-location "/" \
    "$WORK_DIR/pkgs/MIDISplitter_Guide.pkg"
echo -e "${GREEN}Guide component pkg built${NC}"
echo ""

# ── Build final product installer ────────────────────────────────────────────
echo "Building final installer..."

OUTPUT_PKG="$EXPORTS_DIR/${INSTALLER_NAME}.pkg"

productbuild \
    --distribution "$INSTALLER_DIR/distribution.xml" \
    --resources "$INSTALLER_DIR" \
    --package-path "$WORK_DIR/pkgs" \
    "$OUTPUT_PKG"

echo ""
echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}Installer built successfully!${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""
echo "Output: $OUTPUT_PKG"
echo ""

# ── Cleanup ──────────────────────────────────────────────────────────────────
rm -rf "$WORK_DIR"
echo "Scratch files cleaned up."
echo ""

if [ "$GUIDE_AVAILABLE" = false ]; then
    echo -e "${YELLOW}Reminder: Add your PDF guide to:${NC}"
    echo "  Resources/MIDI_Splitter_User_Guide.pdf"
    echo "Then re-run this script to include it in the installer."
    echo ""
fi
