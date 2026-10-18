# Build Notes for MIDI Splitter Plugin

Use the platform-specific build scripts for normal development. `build.sh` and `build_quiet.sh` are compatibility wrappers for the macOS build.

## ⚠️ CRITICAL CONFIGURATION RULES

### Audio Unit (AU) Plugin Type

**NEVER change `AU_MAIN_TYPE` from `kAudioUnitType_MusicDevice`**

- The AU plugin MUST be built as an **Instrument (MusicDevice)** type
- DO NOT change it to `kAudioUnitType_MIDIProcessor` or any other type
- Logic Pro and other DAWs **require** this to be a MusicDevice type for proper MIDI routing
- Changing this will **break AU functionality** in Logic Pro

**Location in code:**
- `CMakeLists.txt`: `AU_MAIN_TYPE kAudioUnitType_MusicDevice`

### IS_MIDI_EFFECT Flag

**Current setting: `TRUE`**

- `IS_MIDI_EFFECT` is a JUCE flag that affects MIDI routing behavior and VST3 categorization
- **It does NOT control AU categorization** - that's handled by `AU_MAIN_TYPE`
- For this plugin, we keep it as `TRUE` because the plugin does process MIDI effects (note distribution)
- The AU will still appear under **Instruments** in Logic because `AU_MAIN_TYPE` is set to `MusicDevice`

**Location in code:**
- `CMakeLists.txt`: `IS_MIDI_EFFECT TRUE`

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

### Cross-platform build scripts

```bash
./Scripts/build_macos.sh   # build/test macOS VST3 and AU
./Scripts/build_windows.sh # build/test Windows x64 VST3 and installer via the configured VM
./Scripts/build_all.sh     # check Windows VM readiness, then build both platforms
```

The cross-platform scripts keep CMake state under `build/macOS/` and `build/Windows/` and stage artifacts in `builds/macOS/` and `builds/Windows/`. Windows VM connection settings belong in the Git-ignored `.windows-vm.local`; see [docs/WINDOWS_BUILD.md](docs/WINDOWS_BUILD.md). Never put a private SSH key in the repository or transfer archive.

MIDI Splitter is available for macOS and Windows: macOS builds VST3 and AU; Windows builds x64 VST3. The Windows installer is unsigned and installs only the VST3 bundle if run.

The Windows installer installs the VST3 bundle to the standard shared VST3 folder and requires administrator access. Automated builds do not run installers or install plugins.

### For Manual/Interactive Use:
```bash
# macOS plugins and tests
./Scripts/build_macos.sh

# macOS installer package (after the macOS build)
./Scripts/build_installer.sh

# macOS DMG with uninstaller
./Scripts/build_dmg.sh
```

### For Automation/AI Tool Use:
```bash
# The platform scripts already report concise, actionable build output.
./Scripts/build_all.sh
./Scripts/build_installer.sh
./Scripts/build_dmg.sh
```

## Build System Safeguards (Added v0.0.17 - Oct 2026)

**Issue:** Debug builds were accidentally packaged, causing 3x file size bloat (9MB vs 3MB).

**Fix:** Added three-layer validation system:
1. `CMakeLists.txt` - Defaults to Release build if unspecified
2. `build_installer.sh` - Rejects binaries >6MB before packaging
3. `build_dmg.sh` - Rejects packages >6MB before DMG creation

The macOS installer and DMG scripts reject unexpectedly large Release artifacts (over 6 MB) as a safeguard against accidentally packaging a Debug build. Build macOS plugins with `./Scripts/build_macos.sh` before creating distribution packages.

---

## Version Updates

The project version is defined by `project(MIDISplitterPlugin VERSION ...)` in `CMakeLists.txt`. JUCE plugin metadata, the plugin UI, and the Windows installer use this same three-component version (for example, `0.0.17`).

Platform build scripts run `Scripts/generate_version.sh` before building so the UI's displayed version and build number are refreshed from the project version. The combined build refreshes once so macOS and Windows artifacts share the same build number.
