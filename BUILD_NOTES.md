# Build Notes for MIDI Splitter Plugin

## ⚠️ CRITICAL CONFIGURATION RULES

### Audio Unit (AU) Plugin Type

**NEVER change `AU_MAIN_TYPE` from `kAudioUnitType_MusicDevice`**

- The AU plugin MUST be built as an **Instrument (MusicDevice)** type
- DO NOT change it to `kAudioUnitType_MIDIProcessor` or any other type
- Logic Pro and other DAWs **require** this to be a MusicDevice type for proper MIDI routing
- Changing this will **break AU functionality** in Logic Pro

**Location in code:**
- `CMakeLists.txt` line ~27: `AU_MAIN_TYPE kAudioUnitType_MusicDevice`

### IS_MIDI_EFFECT Flag

**Current setting: `TRUE`**

- `IS_MIDI_EFFECT` is a JUCE flag that affects MIDI routing behavior and VST3 categorization
- **It does NOT control AU categorization** - that's handled by `AU_MAIN_TYPE`
- For this plugin, we keep it as `TRUE` because the plugin does process MIDI effects (note distribution)
- The AU will still appear under **Instruments** in Logic because `AU_MAIN_TYPE` is set to `MusicDevice`

**Location in code:**
- `CMakeLists.txt` line ~23: `IS_MIDI_EFFECT TRUE`

### Summary: Configuration for Logic Pro compatibility

```cmake
IS_SYNTH TRUE
IS_MIDI_EFFECT TRUE  # ← JUCE MIDI handling flag (doesn't affect AU categorization)
AU_MAIN_TYPE kAudioUnitType_MusicDevice  # ← This determines AU shows under Instruments
```

**Result:** The AU plugin appears under **Instruments** in Logic Pro because of `AU_MAIN_TYPE`, regardless of the `IS_MIDI_EFFECT` setting.

### VST3 Plugin Type

VST3 uses different categorization and is configured correctly as:
- `VST3_CATEGORIES "Instrument" "MIDI"`
- This is separate from AU and does not have the same restriction

---

## Build Commands

```bash
# Full clean build
./Scripts/build.sh

# Build installer package
./Scripts/build_installer.sh

# Build DMG with uninstaller
./Scripts/build_dmg.sh
```

## Version Updates

Version is controlled in `CMakeLists.txt`:
- Line 3: `project(MIDISplitterPlugin VERSION x.x.x)`
- Line 18: `VERSION "x.x.x"`

Build number auto-increments on each build via `Scripts/generate_version.sh`
