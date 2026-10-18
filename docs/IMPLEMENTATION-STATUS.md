# Implementation Status

**Last verified:** 2026-10-17
**Project/plugin version:** `0.0.17`
**Framework:** JUCE 8.0.4
**Build system:** CMake 3.22+
**Supported platforms:** macOS and Windows

## Available builds

- macOS VST3 and AU
- Windows x64 VST3 (built with native MSVC on the ARM64 Windows 11 VM)
- macOS `.pkg`/`.dmg` packaging scripts and a Windows x64 Inno Setup installer

The CMake project version is the source of truth for JUCE plugin metadata and
the UI's generated version header. The Windows installer uses the same
three-component version for its application and file-version metadata (for
example, `0.0.17`, not `0.0.17.0`).

## Implemented functionality

- Distributes incoming MIDI notes across 2-16 voices, mapped to MIDI channels
  1-16.
- Sorts simultaneous notes by pitch and assigns them using the distribution
  algorithm.
- Tracks sustained notes across buffers and broadcasts Control Change messages.
- Provides selectable filtering for Modulation (CC1), Expression (CC11), and
  Volume (CC7).
- Persists plugin state and exposes bypass, voice count, and CC filters as
  automatable parameters.
- Builds platform-appropriate wrappers from shared source: VST3 on both
  platforms and AU on macOS.

## Verification and limitations

- The latest combined build passed **79/79 CTest tests on macOS** and
  **79/79 on Windows**.
- The Windows VST3 binary was verified as PE32+ AMD64/x64.
- macOS VST3 and AU bundle metadata and Windows VST3 metadata report version
  `0.0.17`.
- The Windows installer was rebuilt with three-component version metadata.
- The project owner has confirmed that the Windows installation works.
- Automated processor tests do not load a DAW or validate the AU wrapper.
  No DAW/version compatibility matrix is claimed.
- macOS artifacts are ad-hoc signed, not Developer ID signed or notarized.
  Windows plugin and installer artifacts are unsigned.
- Vendored VST3 SDK compiler warnings remain; first-party warnings were
  addressed.

See [WINDOWS_BUILD.md](./WINDOWS_BUILD.md) for the native Windows build setup,
[BUGS.md](./BUGS.md) for tracked issues, and [FEATURE-IDEAS.md](./FEATURE-IDEAS.md)
for the feature backlog.
