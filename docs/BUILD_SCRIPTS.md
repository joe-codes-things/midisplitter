# Build Scripts

The scripts in `scripts/` build the active project.
Run them from the workspace root.

## Platform Build Scripts

### `build_macos.sh`
- Configures and builds macOS VST3 and AU plugins in `build/<branch>/macOS/`
- Runs the CTest suite and ad-hoc signs the local plugin bundles
- Stages non-main branch plugins in `exports/development-builds/macOS/`
- Names non-main branch plugins separately from production so they can coexist in a DAW

### `build_windows.sh`
- Reads SSH settings from the ignored `.windows-vm.local` or the environment
- Checks the native Windows VM toolchain, including ARM64-hosted MSVC tools, x64 target libraries, Windows SDK, CMake, and Inno Setup
- Builds and tests an x64 VST3 with native MSVC in the VM's branch-specific project workspace
- Retrieves production output into `exports/production-builds/Windows/` on `main`, or development output into `exports/development-builds/Windows/` on other branches
- Uses a separate Windows VM workspace and development plugin/installer identity on non-main branches
- `--check-only` verifies SSH access and VM readiness without transferring source or building

### `build_all.sh`
Checks Windows VM readiness, then builds/tests macOS VST3/AU and builds/tests/packages the Windows x64 VST3.

### `build.sh` / `build_quiet.sh`
Compatibility wrappers for `build_macos.sh`.

### `build_installer.sh`
Creates an unsigned macOS `.pkg` installer under
`exports/production-builds/macOS/Installers/` containing:
- VST3 plugin → `/Library/Audio/Plug-Ins/VST3/`
- AU plugin → `/Library/Audio/Plug-Ins/Components/`
- PDF guide (if available) → `~/Documents/Nine Lives Audio/`

### `build_dmg.sh`
Creates the macOS `.dmg` disk image under
`exports/production-builds/macOS/Installers/` containing:
- The .pkg installer
- Uninstaller.app
- README.md
- PDF User Guide (if available)

## Utility Scripts

### `sign_and_install.sh`
Signs and installs the production plugins to system directories. Refuses to run outside `main`.

### `validate_au.sh` / `validate_vst3.sh`
Validation scripts to test plugins using system tools (auval, pluginval).

### `generate_version.sh`
Generates `source/BuildVersion.h` from the version in
`CMakeLists.txt` and increments the local build
number. Platform build scripts run it automatically; the combined build runs
it once for both platforms.

### `goto-*.sh`
Convenience scripts to navigate to the current branch's build directories.

## Complete Build Workflow

```bash
# Build both plugin platforms from the current branch:
./scripts/build_all.sh

# Build macOS distribution packages (main branch only):
./scripts/build_installer.sh
./scripts/build_dmg.sh

# Or build a single platform:
./scripts/build_macos.sh
./scripts/build_windows.sh
```
