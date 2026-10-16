#!/bin/bash
# uninstall.sh — core uninstall logic for MIDI Splitter
# Called by the Uninstaller.app wrapper with admin privileges.

VERSION_GLOB="*"  # removes any installed version

VST3_SYSTEM="/Library/Audio/Plug-Ins/VST3/MIDI Splitter.vst3"
AU_SYSTEM="/Library/Audio/Plug-Ins/Components/MIDI Splitter.component"
VST3_USER="$HOME/Library/Audio/Plug-Ins/VST3/MIDI Splitter.vst3"
AU_USER="$HOME/Library/Audio/Plug-Ins/Components/MIDI Splitter.component"

# Also remove versioned exports that the installer may have placed
VST3_VERSIONED_GLOB="/Library/Audio/Plug-Ins/VST3/nla_midisplitter_v*.vst3"
AU_VERSIONED_GLOB="/Library/Audio/Plug-Ins/Components/nla_midisplitter_v*.component"

GUIDE_DIR="$HOME/Documents/Nine Lives Audio"

removed=0
errors=0

remove_item() {
    local path="$1"
    if [ -e "$path" ] || [ -d "$path" ]; then
        rm -rf "$path" && removed=$((removed + 1)) || errors=$((errors + 1))
    fi
}

# System-level (requires sudo — called via osascript do shell script ... with administrator privileges)
remove_item "$VST3_SYSTEM"
remove_item "$AU_SYSTEM"

# Versioned names
for f in $VST3_VERSIONED_GLOB; do remove_item "$f"; done
for f in $AU_VERSIONED_GLOB;   do remove_item "$f"; done

# User-level (no sudo needed)
remove_item "$VST3_USER"
remove_item "$AU_USER"

# PDF guide folder — only remove if it only contains NLA files
if [ -d "$GUIDE_DIR" ]; then
    rm -f "$GUIDE_DIR/MIDI_Splitter_User_Guide.pdf"
    rm -f "$GUIDE_DIR/MIDI_Splitter_User_Guide_placeholder.txt"
    # Remove folder only if now empty
    rmdir "$GUIDE_DIR" 2>/dev/null || true
fi

echo "$removed items removed, $errors errors"
exit $errors
