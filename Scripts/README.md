# Build Scripts

## Platform Build Scripts

### `build_macos.sh`
- Configures and builds macOS VST3 and AU plugins in `build/macOS/`
- Runs the CTest suite and ad-hoc signs the local plugin bundles
- Stages plugins in `builds/macOS/`

### `build_windows.sh`
- Reads SSH settings from the ignored `.windows-vm.local` or the environment
- Checks the native Windows VM toolchain, including ARM64-hosted MSVC tools, x64 target libraries, Windows SDK, CMake, and Inno Setup
- Builds and tests an x64 VST3 with native MSVC in the VM's project-specific workspace
- Creates an unsigned Windows installer and retrieves it alongside the plugin into `builds/Windows/`
- `--check-only` verifies SSH access and VM readiness without transferring source or building

### `build_all.sh`
Checks Windows VM readiness, then builds/tests macOS VST3/AU and builds/tests/packages the Windows x64 VST3.

### `build.sh` / `build_quiet.sh`
Compatibility wrappers for `build_macos.sh`.

### `build_installer.sh`
Creates an unsigned macOS `.pkg` installer under `builds/macOS/Installers/` containing:
- VST3 plugin → `/Library/Audio/Plug-Ins/VST3/`
- AU plugin → `/Library/Audio/Plug-Ins/Components/`
- PDF guide (if available) → `~/Documents/Nine Lives Audio/`

### `build_dmg.sh`
Creates the macOS `.dmg` disk image under `builds/macOS/Installers/` containing:
- The .pkg installer
- Uninstaller.app
- README.md
- PDF User Guide (if available)

## Utility Scripts

### `sign_and_install.sh`
Quick script for development - signs and installs plugins to system directories.

### `validate_au.sh` / `validate_vst3.sh`
Validation scripts to test plugins using system tools (auval, pluginval).

### `generate_version.sh`
Generates `Source/BuildVersion.h` from the version in `CMakeLists.txt` and increments the local build number. Platform build scripts run it automatically; the combined build runs it once for both platforms.

### `goto-*.sh`
Convenience scripts to navigate to build directories.

## Complete Build Workflow

```bash
# Build both plugin platforms:
./Scripts/build_all.sh

# Build macOS distribution packages:
./Scripts/build_installer.sh
./Scripts/build_dmg.sh

# Or build a single platform:
./Scripts/build_macos.sh
./Scripts/build_windows.sh
```
