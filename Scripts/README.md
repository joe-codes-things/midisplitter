# Build Scripts

## Core Build Scripts

### `build.sh`
Main build script with full detailed output. Use for manual/interactive builds.
- Configures CMake
- Compiles VST3 and AU plugins
- Runs tests
- Generates ~280KB of output

### `build_quiet.sh` ⚠️ USE THIS FOR AI/AUTOMATION
Wrapper for `build.sh` that suppresses output to avoid timeout issues.
- Same functionality as `build.sh`
- Output suppressed
- Just shows success/failure and version info
- **Always use this for automated/AI builds**

### `build_installer.sh`
Creates the .pkg installer containing:
- VST3 plugin → `/Library/Audio/Plug-Ins/VST3/`
- AU plugin → `/Library/Audio/Plug-Ins/Components/`
- PDF guide (if available) → `~/Documents/Nine Lives Audio/`

### `build_dmg.sh`
Creates the .dmg disk image containing:
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
Auto-generates BuildVersion.h with incremented build number. Called by build.sh.

### `goto-*.sh`
Convenience scripts to navigate to build directories.

## Complete Build Workflow

```bash
# For AI/automated builds:
./Scripts/build_quiet.sh
./Scripts/build_installer.sh
./Scripts/build_dmg.sh

# For manual builds:
./Scripts/build.sh
./Scripts/build_installer.sh
./Scripts/build_dmg.sh
```
