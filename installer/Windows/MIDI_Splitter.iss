#ifndef AppVersion
  #error AppVersion must be provided by the Windows build script.
#endif

#ifndef PluginSource
  #error PluginSource must be provided by the Windows build script.
#endif

[Setup]
AppId={{852A9C3E-057C-4D0D-85A8-C023094BE1D5}
AppName=MIDI Splitter
AppVersion={#AppVersion}
AppVerName=MIDI Splitter {#AppVersion}
AppPublisher=Nine Lives Audio
AppPublisherURL=https://github.com/joe-codes-things/midisplitter
AppSupportURL=https://github.com/joe-codes-things/midisplitter
AppUpdatesURL=https://github.com/joe-codes-things/midisplitter
DefaultDirName={commoncf}\VST3
DefaultGroupName=MIDI Splitter
UninstallDisplayName=MIDI Splitter VST3
LicenseFile=..\..\LICENSE
OutputBaseFilename=MIDISplitter_v{#AppVersion}_Windows_x64_Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
VersionInfoCompany=Nine Lives Audio
VersionInfoDescription=MIDI Splitter VST3 installer
VersionInfoProductName=MIDI Splitter
VersionInfoProductVersion={#AppVersion}
VersionInfoVersion={#AppVersion}

[Files]
Source: "{#PluginSource}\*"; DestDir: "{app}\MIDI Splitter.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall MIDI Splitter"; Filename: "{uninstallexe}"
