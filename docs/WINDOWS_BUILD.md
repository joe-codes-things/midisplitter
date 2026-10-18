# Windows Build and VM Setup

## Windows target

The Windows release target is an x64 VST3. This targets the architecture used by most Windows DAWs; the ARM64 Parallels VM is the native build host, not the plugin target. The build uses Visual Studio 2022 MSVC with ARM64-hosted build tools and the x64 compiler/runtime, the Windows SDK, CMake 3.22+, and Inno Setup 6. The project vendors JUCE 8.0.4 and builds VST3 on Windows; Audio Units are built only on Apple platforms.

The VM workspace is isolated at `Coding Projects/MIDISplitterBuild` relative to the Windows user's home. It contains a synchronized source mirror, `build/Windows/` CMake state, and `builds/Windows/` generated plugin/installer artifacts. The ScratchPad project workspace is not used. If the project source path changes and its CMake cache becomes stale, the Windows build script removes only its marked, project-owned generated build directory before reconfiguring.

## Local SSH setup

Use an SSH alias and a project-dedicated private key stored outside the repository, for example in `~/.ssh/`. Authorize only the corresponding public key for the Windows account. Do not copy private keys into the project, VM workspace, source archive, documentation, or build artifacts. The build script uses non-interactive SSH and SCP and does not change SSH or firewall configuration.

Create the ignored local settings file `.windows-vm.local` in the repository root:

```bash
WINDOWS_SSH_TARGET='your-windows-vm-alias'
WINDOWS_SSH_KEY="$HOME/.ssh/your-project-windows-vm"
```

Alternatively, set `WINDOWS_SSH_TARGET` and `WINDOWS_SSH_KEY` in the environment. Configure the VM address, Windows account, key, and SSH options in the local SSH configuration rather than committing machine-specific connection details. The ignored settings file and `build/Windows/transfer/` are excluded from Git.

Check connectivity and required VM tools without transferring the source:

```bash
./Scripts/build_windows.sh --check-only
```

## Build

```bash
./Scripts/build_windows.sh
```

The script sends only tracked and non-ignored project files, builds on Windows with the Visual Studio 2022 generator (`-A x64`, ARM64-hosted tools), runs the project tests, verifies that the VST3 binary is an AMD64 Windows PE, and compiles the installer. It stages files in a unique, per-run home-relative transfer directory, retrieves that exact directory, and removes the temporary files on success or failure. Cleanup failures are reported.

Build both supported plugin platforms with:

```bash
./Scripts/build_all.sh
```

This checks VM SSH access and native build prerequisites before building macOS VST3/AU and Windows x64 VST3.

## Outputs and installer behavior

- Plugin: `builds/Windows/VST3/MIDI Splitter.vst3`
- Installer: `builds/Windows/Installers/MIDISplitter_v0.0.17_Windows_x64_Setup.exe`
- Build manifest: `builds/Windows/BUILD_INFO.txt`

The installer is unsigned. When a user runs it, it installs only `MIDI Splitter.vst3` under the standard shared VST3 folder (`%CommonProgramFiles%\VST3`) and requires administrator access. Building never runs the installer, installs a plugin, or writes to system plugin directories. The project owner has since confirmed that the Windows installation works. The automated build does not run the installer or a DAW, and no DAW/version compatibility matrix is claimed.
