# MIDI Splitter Plugin - Documentation Index

This directory is the single shared home for version-controlled MIDI Splitter
project documentation. The plugin is available for macOS and Windows: VST3 on
both platforms, and AU on macOS.

## Current State

- The latest macOS CTest run passed all 81 tests. The Windows suite's last
  recorded combined run passed 79/79 tests; Windows was not rerun during the
  workspace reorganization.
- The plugin's JUCE metadata and displayed version are generated from the CMake project version.
- Local CMake build state is separated by branch under `build/`.
- Production artifacts from `main` are staged in `exports/production-builds/`; development artifacts stay under `exports/development-builds/`.
- The project has one Git repository at the workspace root. Documentation, source, tests, build tooling, installer definitions, and the JUCE submodule all live in their project folders under that root.
- There is one checked-out `docs/` directory. Switching Git branches changes those files in place; it does not create separate production and development documentation copies.
- The project owner has confirmed that the Windows installation works. The automated build does not run the installer or a DAW, and no DAW/version compatibility matrix is claimed.
- AU host compatibility is not established by the in-process processor tests.

## Key Documents

- `BUILD_SCRIPTS.md`: build, packaging, validation, and utility script workflows.
- `WINDOWS_BUILD.md`: Windows VM setup, native build requirements, commands, and outputs.
- `PROJECT-ONBOARDING.md`: current project structure, build commands, and artifact paths.
- `IMPLEMENTATION-STATUS.md`: current implementation, verification, and remaining validation limits.
- `BUGS.md`: tracked bug reports and resolved verification issues.
- `FEATURE-IDEAS.md`: feature backlog.
- `MIDI-Explode-Algorithm.md`: MIDI channel/voice assignment algorithm.
- `Plugin UI Asset Creation.md`: plugin UI asset workflow.
- `Plugins-Comms-Guide.md`: inter-plugin communication notes.
- `Sibelius-explode-reduce-guide.md`: Sibelius usage guidance.
- `ui-design/`: visual design specification and gallery.

## Source Research

Additional exploratory material is kept locally in the ignored `research/` directory.

Active plugin implementation lives in `source/`, with tests in `tests/`.
