#!/bin/bash
# build_dmg.sh
# Builds the Uninstaller.app, then packages it alongside the .pkg installer into a .dmg.
# Run after build_installer.sh.
# Requires: osacompile, hdiutil (both built into macOS)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
EXPORTS_DIR="$PROJECT_DIR/exports"
INSTALLER_DIR="$PROJECT_DIR/installer"
WORK_DIR="$PROJECT_DIR/dmg_work"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# ── Resolve version ──────────────────────────────────────────────────────────
VERSION=$(grep "project(MIDISplitterPlugin VERSION" "$PROJECT_DIR/CMakeLists.txt" \
          | sed 's/.*VERSION \([0-9.]*\).*/\1/')

PKG_NAME="MIDISplitter_v${VERSION}_macOS.pkg"
DMG_NAME="MIDISplitter_v${VERSION}_macOS.dmg"
PKG_PATH="$EXPORTS_DIR/$PKG_NAME"

echo "========================================="
echo "MIDI Splitter DMG Builder"
echo "Version: $VERSION"
echo "========================================="
echo ""

# ── Preflight ────────────────────────────────────────────────────────────────
if [ ! -f "$PKG_PATH" ]; then
    echo -e "${RED}Error: Installer pkg not found at:${NC}"
    echo "  $PKG_PATH"
    echo "Run ./Scripts/build_installer.sh first."
    exit 1
fi

# ── Clean work dir ───────────────────────────────────────────────────────────
rm -rf "$WORK_DIR"
mkdir -p "$WORK_DIR/dmg_contents"

# ── Build Uninstaller.app ────────────────────────────────────────────────────
echo "Building Uninstaller.app..."

APP_STAGE="$WORK_DIR/Uninstaller.app"
APP_RESOURCES="$APP_STAGE/Contents/Resources"

# osacompile requires the output path to not already exist and prefers a flat location
OSAC_OUT="$WORK_DIR/UninstallerRaw.app"

osacompile \
    -o "$OSAC_OUT" \
    "$INSTALLER_DIR/uninstaller/UninstallerApp.applescript"

# Copy the uninstall shell script into Resources
mkdir -p "$OSAC_OUT/Contents/Resources"
cp "$INSTALLER_DIR/uninstaller/uninstall.sh" "$OSAC_OUT/Contents/Resources/uninstall.sh"
chmod +x "$OSAC_OUT/Contents/Resources/uninstall.sh"

# Patch Info.plist display name
/usr/libexec/PlistBuddy -c "Set :CFBundleName 'MIDI Splitter Uninstaller'" \
    "$OSAC_OUT/Contents/Info.plist" 2>/dev/null || true
/usr/libexec/PlistBuddy -c "Set :CFBundleDisplayName 'MIDI Splitter Uninstaller'" \
    "$OSAC_OUT/Contents/Info.plist" 2>/dev/null || true

# Rename to final app name
mv "$OSAC_OUT" "$APP_STAGE"

echo -e "${GREEN}Uninstaller.app built${NC}"
echo ""

# ── Generate user manual PDF ─────────────────────────────────────────────────
echo "Generating user manual PDF..."

MANUAL_SCRIPT="$SCRIPT_DIR/build_manual_pdf.py"
if [ -f "$MANUAL_SCRIPT" ]; then
    python3 "$MANUAL_SCRIPT"
    echo -e "${GREEN}User manual PDF generated${NC}"
else
    echo -e "${YELLOW}Warning: build_manual_pdf.py not found — PDF may be stale.${NC}"
fi
echo ""

# ── Stage DMG contents ───────────────────────────────────────────────────────
echo "Staging DMG contents..."

DMG_STAGE="$WORK_DIR/dmg_contents"

# Installer pkg
cp "$PKG_PATH" "$DMG_STAGE/$PKG_NAME"

# Uninstaller app
cp -R "$APP_STAGE" "$DMG_STAGE/MIDI Splitter Uninstaller.app"

# User manual PDF (if present)
USER_MANUAL_PDF="$PROJECT_DIR/Resources/MIDI_Splitter_User_Guide.pdf"
if [ -f "$USER_MANUAL_PDF" ]; then
    cp "$USER_MANUAL_PDF" "$DMG_STAGE/MIDI Splitter User Guide.pdf"
    echo "User manual included."
else
    echo -e "${YELLOW}Warning: User manual not found at Resources/MIDI_Splitter_User_Guide.pdf — skipping.${NC}"
fi

echo -e "${GREEN}DMG contents staged${NC}"
echo ""

# ── Create DMG ───────────────────────────────────────────────────────────────
echo "Creating DMG..."

TEMP_DMG="$WORK_DIR/temp.dmg"
FINAL_DMG="$EXPORTS_DIR/$DMG_NAME"

# Create a writable DMG from the staged folder
hdiutil create \
    -volname "MIDI Splitter v${VERSION}" \
    -srcfolder "$DMG_STAGE" \
    -ov \
    -format UDRW \
    "$TEMP_DMG"

# Convert to compressed read-only DMG
hdiutil convert "$TEMP_DMG" \
    -format UDZO \
    -imagekey zlib-level=9 \
    -ov \
    -o "$FINAL_DMG"

echo ""
echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}DMG built successfully!${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""
echo "Output: $FINAL_DMG"
SIZE=$(du -sh "$FINAL_DMG" | cut -f1)
echo "Size:   $SIZE"
echo ""

# ── Cleanup ──────────────────────────────────────────────────────────────────
rm -rf "$WORK_DIR"
echo "Scratch files cleaned up."
