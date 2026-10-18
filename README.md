# MIDI Splitter Plugin

A professional MIDI instrument plugin that distributes polyphonic MIDI input across 16 MIDI channels, enabling advanced voice management and orchestration workflows in your DAW.

MIDI Splitter is available for **macOS and Windows**: VST3 is supported on both platforms, and Audio Units (AU) are supported on macOS.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows-lightgrey)]()

## Overview

MIDI Splitter is a MIDI processing plugin that intelligently distributes notes from a single MIDI input across 16 MIDI channels. Perfect for:

- **Orchestral arrangements** - Distribute chord voicings across multiple instrument instances
- **Layered sounds** - Split polyphonic input across different synthesizers
- **Voice management** - Organize complex MIDI performances with automatic voice allocation
- **Live performance** - Real-time note distribution with minimal latency

### Key Features

- **2-16 configurable voices** - Flexible voice distribution across full MIDI channel range
- **Intelligent pitch sorting** - Highest notes first (descending order)
- **All CC data sent to all voices** - Sustain, modulation, expression, and all other CC messages
- **Selective CC filtering** - User-controllable Modulation (CC1), Expression (CC11), and Volume (CC7) transmission
- **State persistence** - Full DAW project save/restore support
- **Low latency** - Minimal processing overhead
- **Cross-platform plugins** - VST3 on macOS and Windows; Audio Units on macOS

## Quick Start

### Installation

1. Download the installer or plugin build for your platform
2. Install or copy the plugin to your DAW's plugin folder:
   - **macOS VST3:** `~/Library/Audio/Plug-Ins/VST3/`
   - **macOS AU:** `~/Library/Audio/Plug-Ins/Components/`
   - **Windows x64 VST3:** `%CommonProgramFiles%\VST3\` (the Windows installer requests administrator access)
3. Rescan plugins in your DAW
4. Load MIDI Splitter as an instrument

### Basic Workflow

1. Load MIDI Splitter as an instrument on a MIDI track
2. Configure voice count (2-16 voices)
3. Create separate instrument tracks for each voice you want to use
4. Route MIDI from MIDI Splitter to each voice track, filtered by MIDI channel
5. Load your instruments on each voice track
6. Play MIDI - notes are automatically distributed across channels

**Note:** Voices are numbered 1-16 and correspond to MIDI channels 1-16. Voice 1 outputs on Channel 1, Voice 2 on Channel 2, etc.

### Logic Pro Setup

1. Create an Instrument track and load MIDI Splitter
2. Set the desired number of voices (e.g., 4)
3. Create additional Instrument tracks matching the number of voices
4. On each voice track, open Track Inspector and configure:
   - **Internal MIDI In** → **Instrument Output** → Select your MIDI Splitter track
   - **MIDI In Channel** → Set to voice number (1, 2, 3, etc.)
5. Load your instruments on each voice track
6. Place your raw/unsplit MIDI on the MIDI Splitter track
7. Record-enable all voice tracks
8. Press record - MIDI will be distributed and recorded to each voice track

### Cubase Setup

1. Load an Instrument track or MIDI track and put MIDI Splitter on it
2. Select how many voices you want in the MIDI Splitter plugin (e.g., 4)
3. Create additional MIDI tracks or Instrument tracks matching the number of voices you selected
4. On each voice track:
   - Set **Input Routing** → **MIDI Splitter - MIDI Output**
   - Set **MIDI Channel** to a different channel for each voice, starting from Channel 1 and ascending (Voice 1 = Channel 1, Voice 2 = Channel 2, etc.)
5. Record-enable all voice tracks
6. Press record - MIDI will be distributed and recorded to each voice track

### Ableton Live Setup

1. Create MIDI track and load MIDI Splitter plugin
2. Set voice count
3. Create voice MIDI tracks and load your instruments
4. On each voice track:
   - Set **MIDI From** → **MIDI Splitter**
   - Set **Channel** to the voice number (1, 2, 3, etc.)
   - Set **Monitor** to **In**
5. Arm the MIDI Splitter track and play

**Note:** Voices correspond to MIDI channels 1-16.

## How It Works

### Distribution Process

The plugin receives MIDI input and:

1. Buffers incoming notes within a time window
2. Sorts notes by pitch (highest first)
3. Distributes notes across voices using ratio-based algorithm
4. Outputs each voice on its corresponding MIDI channel (Voice 1 = Channel 1, etc.)
5. Broadcasts Control Change messages to all channels
6. Tracks sustained notes for proper note-off pairing

### Distribution Algorithm

**Example:** 5 notes, 4 voices

```
Input:  C5, A4, G4, E4, C4
Sorted: C5, A4, G4, E4, C4 (descending pitch)

Distribution:
Voice 1 (Ch 1): C5, C4  (2 notes - gets extra note)
Voice 2 (Ch 2): A4      (1 note)
Voice 3 (Ch 3): G4      (1 note)
Voice 4 (Ch 4): E4      (1 note)
```

## User Interface

### Controls

- **Voice Count Selector** - Set number of voices (2-16)
- **CC Filter Checkboxes** - Enable/disable Modulation (CC1), Expression (CC11), and Volume (CC7) transmission
  - **Mod** - Controls CC1 (Modulation Wheel) transmission - Neon green glow when enabled
  - **Exp** - Controls CC11 (Expression) transmission - Deep amber glow when enabled
  - **Vol** - Controls CC7 (Volume) transmission - Electric blue glow when enabled
  - All default to enabled (checked)
  - Dark/dim when disabled

The plugin features a clean, minimal design focused on core functionality with a small window size (400x180px).

## Building from Source

> **⚠️ CRITICAL:** The macOS AU plugin MUST be built as `kAudioUnitType_MusicDevice` (Instrument type). Never change `AU_MAIN_TYPE` in CMakeLists.txt to `kAudioUnitType_MIDIProcessor` or any other type. This will break Logic Pro compatibility. See [BUILD_NOTES.md](BUILD_NOTES.md) for details.

### Prerequisites

- CMake 3.22 or higher
- C++17 compatible compiler
  - macOS: Xcode 12+
  - Windows: Visual Studio 2022 C++ Build Tools and the Windows SDK; the automated Windows build uses a configured Windows 11 VM over SSH
- JUCE framework (included)

See [Windows build and VM setup](docs/WINDOWS_BUILD.md) for the native Windows toolchain, local SSH configuration, and VM workspace details.

### Build macOS plugins

```bash
./Scripts/build_macos.sh
```

This configures and tests in `build/macOS/`, then stages the plugins in `builds/macOS/`. macOS builds require CMake 3.22+, Xcode command-line tools, and the included JUCE checkout. No explicit minimum macOS deployment target is set in this project; the selected Apple toolchain and JUCE defaults apply.

### Build Windows plugins

```bash
./Scripts/build_windows.sh
```

The script checks SSH and VM build-tool readiness, transfers the current non-ignored project files, builds and tests with native MSVC on the Windows VM, and retrieves the x64 VST3 and unsigned installer. It does not install the plugin or run the installer.

### Build both platforms

```bash
./Scripts/build_all.sh
```

The combined script checks Windows VM readiness first, builds and tests the macOS plugins, and then builds and packages the Windows x64 VST3.

### Build outputs

- **macOS VST3:** `builds/macOS/VST3/MIDI Splitter.vst3`
- **macOS AU:** `builds/macOS/AU/MIDI Splitter.component`
- **Windows x64 VST3:** `builds/Windows/VST3/MIDI Splitter.vst3`
- **Windows x64 installer:** `builds/Windows/Installers/MIDISplitter_v0.0.17_Windows_x64_Setup.exe`
- **Windows VM workspace:** `Coding Projects/MIDISplitterBuild` under the VM user's home directory
- **Tests and generated build state:** `build/macOS/` locally and `build/Windows/` in the VM workspace

macOS builds are ad-hoc signed only, not Developer ID signed or notarized. Windows plugin and installer are unsigned. The installer installs only the VST3 bundle to the standard shared Windows VST3 folder when a user runs it with administrator access. The project owner has confirmed that the Windows installation works. The automated build does not run the installer or a DAW, and no DAW/version compatibility matrix is claimed.

## Testing

### Run All Tests

```bash
ctest --test-dir build/macOS --output-on-failure
```

Use `./Scripts/build_windows.sh` to run the Windows test suite through the configured VM, or `./Scripts/build_all.sh` to build and test both platforms.

### Validation Scripts

```bash
# Validate VST3 plugin
./Scripts/validate_vst3.sh

# Validate AU plugin (macOS only)
./Scripts/validate_au.sh
```

### Test Coverage

- Property-based tests for Control Change broadcasting
- Sender MIDI output routing by voice channel
- Sustained notes across buffers
- State persistence (DAW save/restore)
- Plugin validation (structure & metadata)

The automated processor tests verify channel-routed MIDI output; they do not load a
DAW or the Audio Unit wrapper. AU host compatibility must be validated separately
with an AU-aware host or Apple's Audio Unit validation tools.

## Performance

### Resource Usage

- **CPU:** Minimal overhead on modern processors
- **Memory:** Approximately 1MB per instance
- **Maximum Polyphony:** 128 simultaneous notes

## License

MIT License - see [LICENSE](LICENSE) file for details.

Copyright (c) 2026 Nine Lives Audio

## Acknowledgments

- **JUCE Framework** - Cross-platform audio plugin framework
- **Google Test** - C++ testing framework
- **VST3 SDK** - Steinberg VST3 specification
- **Audio Units** - Apple Audio Units specification

## Changelog

### Version 0.0.17 (Current)

- 16-voice support (full MIDI channel range)
- Simplified UI with minimal interface
- macOS VST3 and AU support, plus Windows x64 VST3 support
- Control Change broadcasting with selective CC filtering (Modulation CC1, Expression CC11, Volume CC7)
- State persistence
- Automated macOS and Windows test suites pass (79/79 tests on each platform)
- Bug fixes for editor creation, hanging notes, and note timing

---

**Made with care by Nine Lives Audio**

*MIDI Splitter - Intelligent voice distribution for your DAW*
