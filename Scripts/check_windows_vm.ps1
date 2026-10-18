$ErrorActionPreference = 'Stop'
$architecture = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()
$cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
$ctestCommand = Get-Command ctest.exe -ErrorAction SilentlyContinue
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$kitsRoot = (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' `
    -ErrorAction SilentlyContinue).KitsRoot10
$compilerInstallation = $null
$compilerVersion = $null

if ($architecture -ne 'Arm64') {
    throw "This configured VM must run Windows on ARM64; detected $architecture."
}

if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio 2022 Build Tools (vswhere.exe) were not found.'
}

foreach ($installation in (& $vswhere -products '*' -property installationPath)) {
    $toolsets = Get-ChildItem (Join-Path $installation 'VC\Tools\MSVC') `
        -Directory -ErrorAction SilentlyContinue
    foreach ($toolset in $toolsets) {
        $hostArm64X64Compiler = Join-Path $toolset.FullName 'bin\HostARM64\x64\cl.exe'
        $hostArm64Arm64Compiler = Join-Path $toolset.FullName 'bin\HostARM64\arm64\cl.exe'
        $x64RuntimeLibrary = Join-Path $toolset.FullName 'lib\x64\vcruntime.lib'
        $arm64RuntimeLibrary = Join-Path $toolset.FullName 'lib\arm64\vcruntime.lib'
        if ((Test-Path -LiteralPath $hostArm64X64Compiler) -and
            (Test-Path -LiteralPath $hostArm64Arm64Compiler) -and
            (Test-Path -LiteralPath $x64RuntimeLibrary) -and
            (Test-Path -LiteralPath $arm64RuntimeLibrary)) {
            $compilerInstallation = $installation
            $compilerVersion = $toolset.Name
            break
        }
    }
    if ($compilerInstallation) {
        break
    }
}

if (-not $compilerInstallation) {
    throw 'Visual Studio is missing ARM64-hosted MSVC tools plus both ARM64 helper and x64 plugin target compilers/runtimes.'
}
if (-not $cmakeCommand -or -not $ctestCommand) {
    throw 'CMake 3.22+ and CTest must be available on PATH.'
}

$cmakeVersion = [regex]::Match((& $cmakeCommand.Source --version | Select-Object -First 1), `
    'cmake version (\d+\.\d+\.\d+)')
if (-not $cmakeVersion.Success -or
    [version]$cmakeVersion.Groups[1].Value -lt [version]'3.22.0') {
    throw 'CMake 3.22 or newer is required.'
}

$sdkVersion = Get-ChildItem (Join-Path $kitsRoot 'Include') -Directory |
    Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1
if (-not $sdkVersion -or
    -not (Test-Path -LiteralPath (Join-Path $sdkVersion.FullName 'um\Windows.h')) -or
    -not (Test-Path -LiteralPath (Join-Path $kitsRoot "Lib\$($sdkVersion.Name)\um\x64\kernel32.lib"))) {
    throw 'The Windows SDK is missing x64 headers or libraries.'
}

$innoRoots = @(
    'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*',
    'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*',
    'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*'
)
$innoCompiler = $null
foreach ($root in $innoRoots) {
    foreach ($entry in (Get-ItemProperty $root -ErrorAction SilentlyContinue)) {
        if ($entry.DisplayName -like 'Inno Setup*' -and $entry.InstallLocation) {
            $candidate = Join-Path $entry.InstallLocation 'ISCC.exe'
            if (Test-Path -LiteralPath $candidate) {
                $innoCompiler = $candidate
                break
            }
        }
    }
    if ($innoCompiler) {
        break
    }
}
if (-not $innoCompiler) {
    throw 'Inno Setup 6 (ISCC.exe) must be installed for the Windows installer build.'
}

Write-Output "VM architecture: $architecture (native ARM64 build host)"
Write-Output "MSVC toolset: $compilerVersion (ARM64 and x64 targets, ARM64 host tools)"
Write-Output "Visual Studio: $compilerInstallation"
Write-Output "Windows SDK: $($sdkVersion.Name) (x64 libraries)"
Write-Output "CMake: $($cmakeVersion.Groups[1].Value)"
Write-Output 'Inno Setup 6: ready'
