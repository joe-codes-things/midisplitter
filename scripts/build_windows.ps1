param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9a-f]{32}$')]
    [string] $RunId,
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^MIDISplitterBuild(?:-[a-z0-9-]+)?$')]
    [string] $WorkspaceName,
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[a-z0-9_.-]+$')]
    [string] $PluginName,
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[a-z0-9-]+$')]
    [string] $BranchSlug,
    [Parameter(Mandatory = $true)]
    [ValidateSet('production-builds', 'development-builds')]
    [string] $ExportCategory,
    [Parameter(Mandatory = $true)]
    [bool] $DevelopmentBuild
)

$ErrorActionPreference = 'Stop'
$ProjectWorkspace = Join-Path (Join-Path $env:USERPROFILE 'Coding Projects') $WorkspaceName
$WorkspaceMarker = Join-Path $ProjectWorkspace '.midisplitter-build-workspace'
$SourceDirectory = Join-Path $ProjectWorkspace 'source'
$BuildDirectory = Join-Path $ProjectWorkspace 'build\Windows'
$ArtifactsDirectory = Join-Path (Join-Path $ProjectWorkspace 'exports') (Join-Path $ExportCategory 'Windows')
$StagingDirectory = Join-Path $env:USERPROFILE "midisplitter-artifacts-$RunId"
$Succeeded = $false

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Executable,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]] $Arguments
    )

    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Executable failed with exit code $LASTEXITCODE."
    }
}

function Get-InnoSetupCompiler {
    $uninstallRoots = @(
        'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*',
        'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*'
    )

    foreach ($root in $uninstallRoots) {
        $entries = Get-ItemProperty $root -ErrorAction SilentlyContinue
        foreach ($entry in $entries) {
            if ($entry.DisplayName -like 'Inno Setup*' -and $entry.InstallLocation) {
                $compiler = Join-Path $entry.InstallLocation 'ISCC.exe'
                if (Test-Path -LiteralPath $compiler) {
                    return $compiler
                }
            }
        }
    }

    throw 'Inno Setup 6 (ISCC.exe) is required to build the Windows installer.'
}

if (-not (Test-Path -LiteralPath $ProjectWorkspace)) {
    New-Item -ItemType Directory -Path $ProjectWorkspace | Out-Null
    Set-Content -LiteralPath $WorkspaceMarker -Value 'MIDI Splitter Windows build workspace' -Encoding ascii
} elseif (-not (Test-Path -LiteralPath $WorkspaceMarker) -or
          (Get-Content -LiteralPath $WorkspaceMarker -Raw).Trim() -ne 'MIDI Splitter Windows build workspace') {
    throw "Refusing to use the existing, unmarked project workspace: $ProjectWorkspace"
}

try {
    if (Test-Path -LiteralPath $StagingDirectory) {
        throw "The unique artifact directory already exists: $StagingDirectory"
    }

    if (-not (Test-Path -LiteralPath (Join-Path $SourceDirectory 'CMakeLists.txt'))) {
        throw "The synchronized project source is missing: $SourceDirectory"
    }
    New-Item -ItemType Directory -Path $SourceDirectory -Force | Out-Null
    New-Item -ItemType Directory -Path $BuildDirectory -Force | Out-Null
    New-Item -ItemType Directory -Path $ArtifactsDirectory -Force | Out-Null
    New-Item -ItemType Directory -Path $StagingDirectory | Out-Null

    $projectFile = Join-Path $SourceDirectory 'CMakeLists.txt'
    if (-not (Test-Path -LiteralPath $projectFile)) {
        throw "The transferred project is missing CMakeLists.txt: $SourceDirectory"
    }
    Write-Output "Synchronized project source to: $SourceDirectory"

    $cmake = (Get-Command cmake.exe -ErrorAction Stop).Source
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsInstallation = $null
    foreach ($installation in (& $vswhere -products '*' -property installationPath)) {
        $toolsets = Get-ChildItem (Join-Path $installation 'VC\Tools\MSVC') `
            -Directory -ErrorAction SilentlyContinue
        foreach ($toolset in $toolsets) {
            $requiredFiles = @(
                (Join-Path $toolset.FullName 'bin\HostARM64\x64\cl.exe'),
                (Join-Path $toolset.FullName 'bin\HostARM64\arm64\cl.exe'),
                (Join-Path $toolset.FullName 'lib\x64\vcruntime.lib'),
                (Join-Path $toolset.FullName 'lib\arm64\vcruntime.lib')
            )
            if (@($requiredFiles | Where-Object { -not (Test-Path -LiteralPath $_) }).Count -eq 0) {
                $vsInstallation = $installation
                break
            }
        }
        if ($vsInstallation) {
            break
        }
    }
    if (-not $vsInstallation) {
        throw 'A Visual Studio instance with ARM64-hosted ARM64 and x64 toolchains is required.'
    }

    $cacheFile = Join-Path $BuildDirectory 'CMakeCache.txt'
    if (Test-Path -LiteralPath $cacheFile) {
        $cacheLine = Get-Content -LiteralPath $cacheFile |
            Where-Object { $_.StartsWith('CMAKE_HOME_DIRECTORY:INTERNAL=') } |
            Select-Object -First 1
        $cachedSource = if ($cacheLine) { $cacheLine.Substring($cacheLine.IndexOf('=') + 1).TrimEnd('\') } else { '' }
        $expectedSource = $SourceDirectory.TrimEnd('\')
        $cacheText = Get-Content -LiteralPath $cacheFile -Raw
        $instanceMatch = [regex]::Match($cacheText, '(?m)^CMAKE_GENERATOR_INSTANCE:[^=]*=(.*)$')
        $cachedInstance = if ($instanceMatch.Success) { $instanceMatch.Groups[1].Value.TrimEnd('\') } else { '' }

        if (-not [string]::Equals($cachedSource, $expectedSource, [System.StringComparison]::OrdinalIgnoreCase) -or
            ($cachedInstance -and -not [string]::Equals(
                $cachedInstance,
                $vsInstallation,
                [System.StringComparison]::OrdinalIgnoreCase
            ))) {
            $buildMarker = Join-Path $BuildDirectory '.midisplitter-generated-build'
            if (-not (Test-Path -LiteralPath $buildMarker)) {
                throw "The existing CMake cache points to '$cachedSource', and this build tree is not marked as project-generated."
            }
            Remove-Item -LiteralPath $BuildDirectory -Recurse -Force
            New-Item -ItemType Directory -Path $BuildDirectory | Out-Null
        }
    }

    Set-Content -LiteralPath (Join-Path $BuildDirectory '.midisplitter-generated-build') `
        -Value 'MIDI Splitter CMake build tree' -Encoding ascii

    $devCommand = Join-Path $vsInstallation 'Common7\Tools\VsDevCmd.bat'
    if (-not (Test-Path -LiteralPath $devCommand)) {
        throw "Visual Studio developer command script was not found: $devCommand"
    }
    $configureScript = Join-Path $BuildDirectory 'configure_midi_splitter.cmd'
    $configureLines = @(
        '@echo off'
        "call `"$devCommand`" -no_logo -arch=x64 -host_arch=arm64"
        'if errorlevel 1 exit /b %errorlevel%'
        "set `"MIDI_SPLITTER_GIT_BRANCH=$BranchSlug`""
        "`"$cmake`" -S `"$SourceDirectory`" -B `"$BuildDirectory`" -G `"Visual Studio 17 2022`" -A x64 -T `"host=ARM64`" `"-DCMAKE_GENERATOR_INSTANCE=$vsInstallation`" -DCMAKE_BUILD_TYPE=Release -DJUCE_BUILD_EXAMPLES=OFF -DJUCE_BUILD_EXTRAS=OFF"
        'exit /b %errorlevel%'
    )
    Set-Content -LiteralPath $configureScript -Value $configureLines -Encoding ascii
    Invoke-NativeCommand (Join-Path $env:SystemRoot 'System32\cmd.exe') '/d' '/c' $configureScript
    Invoke-NativeCommand $cmake '--build' $BuildDirectory '--config' 'Release' '--parallel' '4'
    $testExitCode = 0
    & (Join-Path (Split-Path $cmake) 'ctest.exe') `
        '--test-dir' $BuildDirectory '--build-config' 'Release' '--output-on-failure'
    if ($LASTEXITCODE -ne 0) {
        $testExitCode = $LASTEXITCODE
        Write-Warning "CTest reported failures (exit code $testExitCode); continuing to stage the built plugin and installer."
    }

    $versionMatch = [regex]::Match(
        (Get-Content -LiteralPath $projectFile -Raw),
        'project\(MIDISplitterPlugin VERSION ([0-9]+\.[0-9]+\.[0-9]+)\)'
    )
    if (-not $versionMatch.Success) {
        throw 'Could not read the project version from CMakeLists.txt.'
    }
    $projectVersion = $versionMatch.Groups[1].Value

    $pluginSource = Join-Path $BuildDirectory "MIDISplitterPlugin_artefacts\Release\VST3\$PluginName.vst3"
    $pluginBinary = Join-Path $pluginSource "Contents\x86_64-win\$PluginName.vst3"
    if (-not (Test-Path -LiteralPath $pluginBinary)) {
        throw "The expected x64 Windows VST3 binary was not built: $pluginBinary"
    }

    $stream = [System.IO.File]::OpenRead($pluginBinary)
    try {
        $dosHeader = New-Object byte[] 64
        if ($stream.Read($dosHeader, 0, $dosHeader.Length) -ne $dosHeader.Length -or
            $dosHeader[0] -ne 0x4d -or $dosHeader[1] -ne 0x5a) {
            throw 'The VST3 binary does not have a valid PE DOS header.'
        }
        $peOffset = [BitConverter]::ToInt32($dosHeader, 0x3c)
        if ($peOffset -lt 64 -or $peOffset -gt $stream.Length - 6) {
            throw 'The VST3 binary contains an invalid PE header offset.'
        }
        $null = $stream.Seek($peOffset, [System.IO.SeekOrigin]::Begin)
        $peHeader = New-Object byte[] 6
        if ($stream.Read($peHeader, 0, $peHeader.Length) -ne $peHeader.Length -or
            $peHeader[0] -ne 0x50 -or $peHeader[1] -ne 0x45 -or
            [BitConverter]::ToUInt16($peHeader, 4) -ne 0x8664) {
            throw 'The VST3 binary is not an x64 (AMD64) Windows PE.'
        }
    } finally {
        $stream.Dispose()
    }

    $stagedPlugin = Join-Path $ArtifactsDirectory "VST3\$PluginName.vst3"
    if (Test-Path -LiteralPath $stagedPlugin) {
        Remove-Item -LiteralPath $stagedPlugin -Recurse -Force
    }
    New-Item -ItemType Directory -Path (Split-Path $stagedPlugin) -Force | Out-Null
    Copy-Item -LiteralPath $pluginSource -Destination $stagedPlugin -Recurse

    $installerScript = Join-Path $SourceDirectory 'installer\Windows\MIDI_Splitter.iss'
    $installerDirectory = Join-Path $ArtifactsDirectory 'Installers'
    New-Item -ItemType Directory -Path $installerDirectory -Force | Out-Null
    $installerCompiler = Get-InnoSetupCompiler
    $installerBaseName = if ($DevelopmentBuild) { "MIDISplitter_${BranchSlug}" } else { 'MIDISplitter' }
    $installerName = "${installerBaseName}_v${projectVersion}_Windows_x64_Setup.exe"
    $installerCompilerArguments = @(
        "/DAppVersion=$projectVersion"
        "/DPluginSource=$stagedPlugin"
        "/DPluginName=$PluginName"
        "/DInstallerBaseName=$installerBaseName"
        "/DDevelopmentBuild=$([int]$DevelopmentBuild)"
        "/O$installerDirectory"
        $installerScript
    )
    Invoke-NativeCommand $installerCompiler @installerCompilerArguments

    $installer = Join-Path $installerDirectory $installerName
    if (-not (Test-Path -LiteralPath $installer)) {
        throw "The Windows installer was not created: $installer"
    }

    New-Item -ItemType Directory -Path (Join-Path $StagingDirectory 'VST3') | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $StagingDirectory 'Installers') | Out-Null
    Copy-Item -LiteralPath $stagedPlugin -Destination (Join-Path $StagingDirectory "VST3\$PluginName.vst3") -Recurse
    Copy-Item -LiteralPath $installer -Destination (Join-Path $StagingDirectory 'Installers')
    $testStatus = if ($testExitCode -eq 0) { 'passed' } else { "FAILED (CTest exit code $testExitCode)" }
    Set-Content -LiteralPath (Join-Path $StagingDirectory 'BUILD_INFO.txt') `
        -Value @(
            "Product: $PluginName"
            "Project version: $projectVersion"
            'Platform: Windows 11 ARM64 VM, native MSVC x64 target'
            'Plugin format: VST3 x64 (PE/AMD64 verified)'
            'Installer: Inno Setup 6; installs the VST3 bundle to the standard common VST3 folder'
            'Code signing: unsigned'
            "Windows tests: $testStatus"
            'DAW validation: not performed'
        ) -Encoding utf8

    Write-Output "Windows x64 VST3: $stagedPlugin"
    Write-Output "Windows installer: $installer"
    Write-Output "Windows VM workspace: $ProjectWorkspace"
    Write-Output "Temporary artifacts ready for retrieval: midisplitter-artifacts-$RunId"
    $Succeeded = $true
} finally {
    if (-not $Succeeded -and (Test-Path -LiteralPath $StagingDirectory)) {
        Remove-Item -LiteralPath $StagingDirectory -Recurse -Force
    }
}
