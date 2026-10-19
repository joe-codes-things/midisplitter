# MIDI Splitter Plugin - Documentation Index

This directory contains selected project documentation for MIDI Splitter. The plugin is available for macOS and Windows: VST3 on both platforms, and AU on macOS.

## Current State

- The current macOS and Windows test suites each pass with 79/79 tests.
- The plugin's JUCE metadata and displayed version are generated from the CMake project version.
- macOS build state is under `build/macOS/`; its AU and VST3 artifacts are staged under `exports/production-builds/macOS/`.
- Windows build state stays in the project-specific VM workspace; its x64 VST3 and unsigned installer are staged under `exports/production-builds/Windows/`.
- The project owner has confirmed that the Windows installation works. The automated build does not run the installer or a DAW, and no DAW/version compatibility matrix is claimed.
- AU host compatibility is not established by the in-process processor tests.

## Key Documents

- `WINDOWS_BUILD.md`: Windows VM setup, native build requirements, commands, and outputs.
- `PROJECT-ONBOARDING.md`: current project structure, development commands, and artifact paths.
- `IMPLEMENTATION-STATUS.md`: current implementation, verification, and remaining validation limits.
- `BUGS.md`: tracked bug reports and resolved verification issues.
- `FEATURE-IDEAS.md`: feature backlog.

## Source Research

Internal research notes and reference material are kept locally and are not part of the selected published documentation.

Active plugin implementation lives in `source/`, with tests in `tests/`.
