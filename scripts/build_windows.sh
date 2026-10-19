#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
REPOSITORY_ROOT="$(git -C "$PROJECT_ROOT" rev-parse --show-toplevel)"
source "$SCRIPT_DIR/branch_config.sh"
CONFIG_FILE="$PROJECT_ROOT/.windows-vm.local"
if [[ -n "${MIDI_SPLITTER_WINDOWS_CONFIG:-}" ]]; then
    CONFIG_FILE="$MIDI_SPLITTER_WINDOWS_CONFIG"
fi
GIT_BRANCH="$(midisplitter_branch_name "$PROJECT_ROOT")"
BRANCH_SLUG="$(midisplitter_branch_slug "$GIT_BRANCH")"
TRANSFER_ROOT="$PROJECT_ROOT/build/$BRANCH_SLUG/Windows/transfer"
PLUGIN_NAME="$(midisplitter_product_name "$GIT_BRANCH")"
EXPORT_CATEGORY="$(midisplitter_export_category "$GIT_BRANCH")"
ARTIFACTS_DIR="$(midisplitter_export_dir "$PROJECT_ROOT" "$GIT_BRANCH")/Windows"
if [[ "$GIT_BRANCH" == "main" ]]; then
    VM_WORKSPACE_NAME="MIDISplitterBuild"
    DEVELOPMENT_BUILD=false
    INSTALLER_BASE_NAME="MIDISplitter"
else
    VM_WORKSPACE_NAME="MIDISplitterBuild-$BRANCH_SLUG"
    DEVELOPMENT_BUILD=true
    INSTALLER_BASE_NAME="MIDISplitter_${BRANCH_SLUG}"
fi
SSH_TARGET="${WINDOWS_SSH_TARGET:-}"
SSH_KEY="${WINDOWS_SSH_KEY:-}"
CHECK_ONLY=false

if [[ "${1:-}" == "--check-only" ]]; then
    CHECK_ONLY=true
elif [[ "$#" -ne 0 ]]; then
    echo "Usage: $0 [--check-only]" >&2
    exit 2
fi

RUN_ID="$(uuidgen | tr -d '-' | tr '[:upper:]' '[:lower:]')"
LOCAL_RUN_DIR="$TRANSFER_ROOT/$RUN_ID"
ARCHIVE="$LOCAL_RUN_DIR/midisplitter-source-$RUN_ID.tar.gz"
REMOTE_ARCHIVE="midisplitter-source-$RUN_ID.tar.gz"
REMOTE_ARTIFACTS="midisplitter-artifacts-$RUN_ID"
REMOTE_BUILD_STARTED=false

if [[ -f "$CONFIG_FILE" ]]; then
    # This file is ignored by Git and may contain machine-specific SSH settings.
    # shellcheck disable=SC1090
    source "$CONFIG_FILE"
    SSH_TARGET="${WINDOWS_SSH_TARGET:-$SSH_TARGET}"
    SSH_KEY="${WINDOWS_SSH_KEY:-$SSH_KEY}"
fi

if [[ -z "$SSH_TARGET" || -z "$SSH_KEY" ]]; then
    cat >&2 <<'EOF'
Error: Configure the Windows VM connection in .windows-vm.local or set:
  WINDOWS_SSH_TARGET
  WINDOWS_SSH_KEY
See docs/WINDOWS_BUILD.md for the local setup instructions.
EOF
    exit 1
fi

if [[ ! -r "$SSH_KEY" ]]; then
    echo "Error: The configured Windows VM SSH key is missing or unreadable." >&2
    exit 1
fi

for tool in ssh scp tar cmake python3 uuidgen; do
    command -v "$tool" >/dev/null || { echo "Error: $tool is required." >&2; exit 1; }
done

SSH_OPTIONS=(-i "$SSH_KEY" -o BatchMode=yes -o ConnectTimeout=10)

encode_powershell() {
    printf '%s' "$1" | iconv -f UTF-8 -t UTF-16LE | base64 | tr -d '\r\n'
}

check_windows_readiness() {
    local bootstrap
    bootstrap="$(encode_powershell '$script = [Console]::In.ReadToEnd(); try { Invoke-Expression $script } catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }')"
    ssh "${SSH_OPTIONS[@]}" "$SSH_TARGET" \
        "powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand $bootstrap" \
        < "$SCRIPT_DIR/check_windows_vm.ps1"
}

cleanup_remote() {
    local cleanup_script cleanup_command cleanup_status=0

    cleanup_script="\$ErrorActionPreference = 'Stop'; \$userHome = [Environment]::GetFolderPath('UserProfile'); foreach (\$name in @('$REMOTE_ARCHIVE', '$REMOTE_ARTIFACTS')) { \$path = Join-Path \$userHome \$name; if (Test-Path -LiteralPath \$path) { Remove-Item -LiteralPath \$path -Recurse -Force -ErrorAction Stop } }; Write-Output 'Temporary Windows transfer files cleaned up.'"
    cleanup_command="$(encode_powershell "$cleanup_script")"
    if ! ssh "${SSH_OPTIONS[@]}" "$SSH_TARGET" \
        "powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand $cleanup_command"; then
        echo "Error: Failed to clean the unique Windows transfer paths for run $RUN_ID." >&2
        cleanup_status=1
    fi
    return "$cleanup_status"
}

cleanup_local() {
    local cleanup_status=0
    if [[ -d "$LOCAL_RUN_DIR" ]]; then
        if ! rm -rf "$LOCAL_RUN_DIR"; then
            echo "Error: Failed to clean the local transfer directory $LOCAL_RUN_DIR." >&2
            cleanup_status=1
        fi
    fi
    return "$cleanup_status"
}

on_exit() {
    local result=$?
    trap - EXIT

    if [[ "$REMOTE_BUILD_STARTED" == true ]]; then
        if ! cleanup_remote; then
            result=1
        fi
    fi
    if ! cleanup_local; then
        result=1
    fi
    exit "$result"
}
trap on_exit EXIT

echo "Checking the Windows VM SSH connection..."
ssh "${SSH_OPTIONS[@]}" "$SSH_TARGET" whoami
check_windows_readiness

if [[ "$CHECK_ONLY" == true ]]; then
    exit 0
fi

if [[ "${MIDI_SPLITTER_VERSION_PREPARED:-0}" != "1" ]]; then
    "$SCRIPT_DIR/generate_version.sh"
fi

mkdir -p "$LOCAL_RUN_DIR"

# Transfer only tracked and non-ignored project files; local credentials, build
# trees, previous artifacts, and unrelated ignored files never enter the archive.
git -C "$REPOSITORY_ROOT" ls-files --cached --others --exclude-standard --full-name -z |
    tar --exclude='*/.git' --exclude='*/.git/**' \
        -czf "$ARCHIVE" -C "$REPOSITORY_ROOT" --null -T -

echo "Transferring the current project source to its dedicated Windows workspace..."
scp "${SSH_OPTIONS[@]}" "$ARCHIVE" "$SSH_TARGET:$REMOTE_ARCHIVE"

REMOTE_BUILD_STARTED=true
remote_script="\$ErrorActionPreference = 'Stop'
\$userHome = [Environment]::GetFolderPath('UserProfile')
\$projectWorkspaces = Join-Path \$userHome 'Coding Projects'
\$workspace = Join-Path \$projectWorkspaces '$VM_WORKSPACE_NAME'
\$workspaceMarker = Join-Path \$workspace '.midisplitter-build-workspace'
\$source = Join-Path \$workspace 'source'
\$incoming = Join-Path \$userHome 'midisplitter-incoming-$RUN_ID'
\$archive = Join-Path \$userHome '$REMOTE_ARCHIVE'
\$markerContents = 'MIDI Splitter Windows build workspace'
if (-not (Test-Path -LiteralPath \$workspace)) {
    New-Item -ItemType Directory -Path \$workspace | Out-Null
    Set-Content -LiteralPath \$workspaceMarker -Value \$markerContents -Encoding ascii
} elseif (-not (Test-Path -LiteralPath \$workspaceMarker) -or (Get-Content -LiteralPath \$workspaceMarker -Raw).Trim() -ne \$markerContents) {
    throw \"Refusing to modify an existing, unmarked workspace: \$workspace\"
}
if (Test-Path -LiteralPath \$incoming) { throw \"The unique source staging directory already exists: \$incoming\" }
if (-not (Test-Path -LiteralPath \$archive)) { throw 'The source transfer archive is missing.' }
try {
    New-Item -ItemType Directory -Path \$incoming | Out-Null
    & (Join-Path \$env:SystemRoot 'System32\\tar.exe') -xzf \$archive -C \$incoming
    if (\$LASTEXITCODE -ne 0) { throw \"Source archive extraction failed with exit code \$LASTEXITCODE.\" }
    New-Item -ItemType Directory -Path \$source -Force | Out-Null
    & (Join-Path \$env:SystemRoot 'System32\\robocopy.exe') \$incoming \$source /MIR /NFL /NDL /NJH /NJS /NP
    if (\$LASTEXITCODE -ge 8) { throw \"Source synchronization failed with robocopy exit code \$LASTEXITCODE.\" }
    \$buildScript = Join-Path \$source 'scripts\\build_windows.ps1'
    if (-not (Test-Path -LiteralPath \$buildScript)) { throw \"Transferred Windows build script is missing: \$buildScript\" }
    & \$buildScript -RunId '$RUN_ID' -WorkspaceName '$VM_WORKSPACE_NAME' -PluginName '$PLUGIN_NAME' -BranchSlug '$BRANCH_SLUG' -ExportCategory '$EXPORT_CATEGORY' -DevelopmentBuild \$$DEVELOPMENT_BUILD
    if (\$LASTEXITCODE -and \$LASTEXITCODE -ne 0) { throw \"Windows build script failed with exit code \$LASTEXITCODE.\" }
} finally {
    if (Test-Path -LiteralPath \$incoming) { Remove-Item -LiteralPath \$incoming -Recurse -Force }
    if (Test-Path -LiteralPath \$archive) { Remove-Item -LiteralPath \$archive -Force }
}"
bootstrap="$(encode_powershell '$script = [Console]::In.ReadToEnd(); try { Invoke-Expression $script } catch { [Console]::Error.WriteLine($_.Exception.Message); exit 1 }')"
ssh "${SSH_OPTIONS[@]}" "$SSH_TARGET" \
    "powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand $bootstrap" \
    <<< "$remote_script"

mkdir -p "$TRANSFER_ROOT/$RUN_ID/artifacts"
echo "Retrieving this run's Windows artifacts..."
scp "${SSH_OPTIONS[@]}" -r "$SSH_TARGET:$REMOTE_ARTIFACTS/." "$TRANSFER_ROOT/$RUN_ID/artifacts/"

VERSION="$(sed -n 's/^project(MIDISplitterPlugin VERSION \([0-9.]*\)).*/\1/p' "$PROJECT_ROOT/CMakeLists.txt" | head -n 1)"
STAGED_PLUGIN="$TRANSFER_ROOT/$RUN_ID/artifacts/VST3/$PLUGIN_NAME.vst3"
STAGED_BINARY="$STAGED_PLUGIN/Contents/x86_64-win/$PLUGIN_NAME.vst3"
STAGED_INSTALLER="$TRANSFER_ROOT/$RUN_ID/artifacts/Installers/${INSTALLER_BASE_NAME}_v${VERSION}_Windows_x64_Setup.exe"

for artifact in "$STAGED_PLUGIN" "$STAGED_BINARY" "$STAGED_INSTALLER"; do
    if [[ ! -e "$artifact" ]]; then
        echo "Error: Expected Windows build artifact is missing: $artifact" >&2
        exit 1
    fi
done

python3 -c '
import pathlib, struct, sys
path = pathlib.Path(sys.argv[1])
data = path.read_bytes()
if len(data) < 70 or data[:2] != b"MZ":
    raise SystemExit("VST3 binary is not a valid PE image (missing MZ header)")
offset = struct.unpack_from("<I", data, 0x3c)[0]
if offset + 6 > len(data) or data[offset:offset + 4] != b"PE\0\0":
    raise SystemExit("VST3 binary is not a valid PE image (missing PE header)")
machine = struct.unpack_from("<H", data, offset + 4)[0]
if machine != 0x8664:
    raise SystemExit(f"Expected x64/AMD64 PE machine 0x8664; found 0x{machine:04x}")
print(f"Verified Windows x64 PE binary: {path}")
' "$STAGED_BINARY"

file "$STAGED_BINARY"

mkdir -p "$ARTIFACTS_DIR/VST3" "$ARTIFACTS_DIR/Installers"
cmake -E rm -rf "$ARTIFACTS_DIR/VST3/$PLUGIN_NAME.vst3"
cmake -E copy_directory "$STAGED_PLUGIN" "$ARTIFACTS_DIR/VST3/$PLUGIN_NAME.vst3"
cp "$STAGED_INSTALLER" "$ARTIFACTS_DIR/Installers/"
BUILD_INFO="$TRANSFER_ROOT/$RUN_ID/artifacts/BUILD_INFO.txt"
cp "$BUILD_INFO" "$ARTIFACTS_DIR/"

printf 'Windows VST3:     %s\nWindows installer: %s\nWindows VM workspace: %s\n' \
    "$ARTIFACTS_DIR/VST3/$PLUGIN_NAME.vst3" \
    "$ARTIFACTS_DIR/Installers/$(basename "$STAGED_INSTALLER")" \
    "$VM_WORKSPACE_NAME"

if grep -q '^Windows tests: FAILED ' "$BUILD_INFO"; then
    echo "Error: The Windows plugin and installer were built, but the Windows CTest suite reported failures." >&2
    exit 1
fi
