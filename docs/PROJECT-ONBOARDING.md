# Project Onboarding

## Project and platform support

MIDI Splitter is a JUCE 8.0.4 / C++17 MIDI instrument plugin. It distributes
incoming notes across 2-16 MIDI channels, with each channel representing a
voice.

- **macOS:** VST3 and Audio Unit (AU)
- **Windows:** x64 VST3
- **Current project and plugin version:** `0.0.17`

The plugin's CMake project version is the source of truth for JUCE plugin
metadata, the version shown in the UI, and the Windows installer. Build scripts
regenerate `Source/BuildVersion.h` from that version.

## Build and test

Requirements and local Windows VM/SSH setup are described in
[WINDOWS_BUILD.md](./WINDOWS_BUILD.md).

```bash
# macOS VST3, AU, and tests
./Scripts/build_macos.sh

# Windows x64 VST3, tests, and installer through the configured VM
./Scripts/build_windows.sh

# Readiness check, then build and test both platforms
./Scripts/build_all.sh
```

The macOS build requires CMake 3.22+, Xcode command-line tools, and the
vendored JUCE checkout. The Windows build uses native MSVC and Inno Setup on
the configured Windows VM. Tests run with CTest; the combined build currently
passes 79/79 tests on each platform.

## Artifact locations

- macOS VST3: `builds/macOS/VST3/MIDI Splitter.vst3`
- macOS AU: `builds/macOS/AU/MIDI Splitter.component`
- macOS PKG/DMG: `builds/macOS/Installers/` (created by the macOS packaging scripts)
- Windows x64 VST3: `builds/Windows/VST3/MIDI Splitter.vst3`
- Windows installer: `builds/Windows/Installers/MIDISplitter_v0.0.17_Windows_x64_Setup.exe`
- macOS CMake state: `build/macOS/`
- Windows CMake state and source mirror: `Coding Projects/MIDISplitterBuild` under the VM user's home

Builds do not install plugins or run installers. macOS bundles are ad-hoc
signed, not Developer ID signed or notarized. The Windows plugin and installer
are unsigned. The project owner has confirmed a successful Windows installation;
automated tests do not validate a DAW host or AU wrapper.

## Source layout

- `Source/`: MIDI processing, plugin processor/editor, UI, IPC, and state handling.
- `Tests/`: CTest-registered processor and support-code tests.
- `Scripts/`: platform build, packaging, validation, and utility scripts.
- `installer/`: macOS and Windows installer definitions.
- `JUCE/`: vendored JUCE framework.
- `docs/`: maintained build, onboarding, status, bug, and feature documentation.
