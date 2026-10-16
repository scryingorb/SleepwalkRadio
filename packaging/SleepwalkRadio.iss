#define ReleaseVersion "1.0.0-rc.1"
#define StageDir SourcePath + "stage"

[Setup]
AppId={{B896426C-6524-4681-AD5C-CDBDDF63A1D8}
AppName=SleepwalkRadio
AppVersion={#ReleaseVersion}
AppVerName=SleepwalkRadio {#ReleaseVersion} - Release Candidate
AppPublisher=scryingorb
DefaultDirName={commoncf64}\VST3\SleepwalkRadio.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
UninstallFilesDir={commonappdata}\SleepwalkRadio\Uninstall
UninstallDisplayName=SleepwalkRadio {#ReleaseVersion} (VST3)
LicenseFile={#StageDir}\LICENSE
InfoBeforeFile={#StageDir}\INSTALL.txt
OutputDir=..\dist
OutputBaseFilename=SleepwalkRadio-{#ReleaseVersion}-Windows-x64-Setup
VersionInfoVersion=1.0.0.0
VersionInfoProductVersion=1.0.0.0
VersionInfoDescription=SleepwalkRadio {#ReleaseVersion} installer
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=no
RestartApplications=no
SetupLogging=yes

[Files]
Source: "{#StageDir}\SleepwalkRadio.vst3\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\LICENSE"; DestDir: "{commonappdata}\SleepwalkRadio"; Flags: ignoreversion
Source: "{#StageDir}\INSTALL.txt"; DestDir: "{commonappdata}\SleepwalkRadio"; Flags: ignoreversion
Source: "{#StageDir}\THIRD_PARTY_NOTICES.md"; DestDir: "{commonappdata}\SleepwalkRadio"; Flags: ignoreversion
Source: "{#StageDir}\licenses\*"; DestDir: "{commonappdata}\SleepwalkRadio\licenses"; Flags: ignoreversion recursesubdirs createallsubdirs

; Inno tracks only files it installs. No wildcard uninstall deletion of VST3 directories.
