; Inno Setup script for VisKey for Windows (MIT License, 1VISION).
; Optional: the portable zip is the primary download. Built by .github/workflows/windows.yml:
;   iscc /DAppVersion=1.1.0-beta.5 /DAppVersionNumeric=1.1.0 /DSourceExe=build\Release\VisKey.exe /DRepoRoot=. /Odist windows\installer\VisKey.iss
; AppVersion is the output of scripts/viskey_version.sh (the release tag without the leading v),
; AppVersionNumeric that of scripts/viskey_version.sh --numeric (for the installer's VERSIONINFO).
;
; Installs per user by default (no admin prompt) into %LOCALAPPDATA%\Programs\VisKey; the user may pick
; "all users" (Program Files) in the dialog. Settings live in HKCU\Software\VisKey.

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef AppVersionNumeric
  ; x.y.z part of AppVersion (VersionInfoVersion only accepts numbers).
  #if Pos("-", AppVersion) > 0
    #define AppVersionNumeric Copy(AppVersion, 1, Pos("-", AppVersion) - 1)
  #elif Pos("+", AppVersion) > 0
    #define AppVersionNumeric Copy(AppVersion, 1, Pos("+", AppVersion) - 1)
  #else
    #define AppVersionNumeric AppVersion
  #endif
#endif
#ifndef RepoRoot
  #define RepoRoot "..\.."
#endif
#ifndef SourceExe
  #define SourceExe RepoRoot + "\build\Release\VisKey.exe"
#endif

[Setup]
AppId={{4298D436-993C-44E9-8944-6FD33424EBEE}
AppName=VisKey
AppVersion={#AppVersion}
AppVerName=VisKey {#AppVersion}
AppPublisher=1VISION
AppPublisherURL=https://github.com/1vision-work/VisKey
AppSupportURL=https://github.com/1vision-work/VisKey/issues
AppUpdatesURL=https://github.com/1vision-work/VisKey/releases
AppCopyright=Copyright (c) 2026 VisKey Project (1VISION). MIT License.
DefaultDirName={autopf}\VisKey
DefaultGroupName=VisKey
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile={#RepoRoot}\LICENSE
OutputBaseFilename=VisKey-{#AppVersion}-Windows-Setup
SetupIconFile={#RepoRoot}\windows\res\VisKey.ico
Uninstallable=yes
UninstallDisplayName=VisKey {#AppVersion}
UninstallDisplayIcon={app}\VisKey.exe,0
; Properties of VisKey-<version>-Windows-Setup.exe itself (same values as windows/res/VisKey.rc).
VersionInfoVersion={#AppVersionNumeric}
VersionInfoProductVersion={#AppVersionNumeric}
VersionInfoTextVersion={#AppVersion}
VersionInfoProductTextVersion={#AppVersion}
VersionInfoCompany=1VISION
VersionInfoProductName=VisKey
VersionInfoDescription=VisKey Setup - Vietnamese input method (Telex/VNI)
VersionInfoCopyright=Copyright (c) 2026 VisKey Project (1VISION). MIT License.
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "startup"; Description: "Khởi động VisKey cùng Windows (Start with Windows)"; Flags: unchecked
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoRoot}\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "{#RepoRoot}\windows\README-Windows.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[InstallDelete]
; Older versions installed the exe as VisKey64.exe (stopped in PrepareToInstall below).
Type: files; Name: "{app}\VisKey64.exe"

[Icons]
Name: "{autoprograms}\VisKey"; Filename: "{app}\VisKey.exe"
Name: "{autodesktop}\VisKey"; Filename: "{app}\VisKey.exe"; Tasks: desktopicon

[Registry]
; Same Run value VisKey.exe writes itself ("Tự khởi động VisKey cùng hệ điều hành Windows").
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "VisKey"; ValueData: """{app}\VisKey.exe"""; Tasks: startup
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "VisKey"; Flags: uninsdeletevalue dontcreatekey
; Keep the in-app checkbox consistent with the startup task.
Root: HKCU; Subkey: "Software\VisKey"; ValueType: dword; ValueName: "LaunchAtStartup"; ValueData: "1"; Tasks: startup
Root: HKCU; Subkey: "Software\VisKey"; Flags: uninsdeletekeyifempty

[Run]
Filename: "{app}\VisKey.exe"; Description: "{cm:LaunchProgram,VisKey}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{sys}\taskkill.exe"; Parameters: "/IM VisKey.exe /F"; Flags: runhidden; RunOnceId: "StopVisKey"

[Code]
// An older install still running as VisKey64.exe would keep its file locked and its hooks active.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Exec(ExpandConstant('{sys}\taskkill.exe'), '/IM VisKey64.exe /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := '';
end;
