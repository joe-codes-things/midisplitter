# MIDI Splitter Plugin

A professional MIDI instrument plugin that distributes polyphonic MIDI input across 16 MIDI channels, enabling advanced voice management and orchestration workflows in your DAW.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()
[![Platform](https://img.shields.io/badge/platform-macOS-lightgrey)]()

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
- **Dual format support** - VST3 and Audio Units

## Quick Start

### Installation

1. Download the latest release for your platform
2. Copy the plugin to your DAW's plugin folder:
   - **macOS VST3:** `~/Library/Audio/Plug-Ins/VST3/`
   - **macOS AU:** `~/Library/Audio/Plug-Ins/Components/`
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

> **⚠️ CRITICAL:** The AU plugin MUST be built as `kAudioUnitType_MusicDevice` (Instrument type). Never change `AU_MAIN_TYPE` in CMakeLists.txt to `kAudioUnitType_MIDIProcessor` or any other type. This will break Logic Pro compatibility. See [BUILD_NOTES.md](BUILD_NOTES.md) for details.

### Prerequisites

- CMake 3.22 or higher
- C++17 compatible compiler
  - macOS: Xcode 12+
- JUCE framework (included)

### Build Steps

```bash
# Clone repository
git clone https://github.com/joe-codes-things/midisplitter.git
cd midisplitter

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run tests
ctest --output-on-failure
```

### Build Outputs

- **VST3:** `build/MIDISplitterPlugin_artefacts/Release/VST3/MIDI Splitter.vst3`
- **AU:** `build/MIDISplitterPlugin_artefacts/Release/AU/MIDI Splitter.component`
- **Tests:** `build/MIDISplitterPluginTests`

## Testing

### Run All Tests

```bash
ctest --output-on-failure
```

### Validation Scripts

```bash
# Validate VST3 plugin
./Scripts/validate_vst3.sh

# Validate AU plugin (macOS only)
./Scripts/validate_au.sh
```

### Test Coverage

- Property-based tests for Control Change broadcasting
- End-to-end integration tests (VST3 & AU)
- Sustained notes across buffers
- State persistence (DAW save/restore)
- Plugin validation (structure & metadata)

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

### Version 0.0.15 (Current)

- 16-voice support (full MIDI channel range)
- Simplified UI with minimal interface
- VST3 and AU support
- Control Change broadcasting with selective CC filtering (Modulation CC1, Expression CC11, Volume CC7)
- State persistence
- Production tested and verified
- Bug fixes for editor creation, hanging notes, and note timing

---

**Made with care by Nine Lives Audio**

*MIDI Splitter - Intelligent voice distribution for your DAW*
