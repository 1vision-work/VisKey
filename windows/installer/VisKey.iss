; Inno Setup script for VisKey for Windows (MIT License, 1VISION).
; Optional: the portable zip is the primary download. Built by .github/workflows/windows.yml:
;   iscc /DAppVersion=1.0.0 /DSourceExe=build\Release\VisKey64.exe /DRepoRoot=. /Odist windows\installer\VisKey.iss
;
; Installs per user by default (no admin prompt) into %LOCALAPPDATA%\Programs\VisKey; the user may pick
; "all users" (Program Files) in the dialog. Settings live in HKCU\Software\VisKey.

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef RepoRoot
  #define RepoRoot "..\.."
#endif
#ifndef SourceExe
  #define SourceExe RepoRoot + "\build\Release\VisKey64.exe"
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
AppCopyright=MIT License. Copyright (c) 2026 VisKey Project.
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
UninstallDisplayIcon={app}\VisKey64.exe
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

[Icons]
Name: "{autoprograms}\VisKey"; Filename: "{app}\VisKey64.exe"
Name: "{autodesktop}\VisKey"; Filename: "{app}\VisKey64.exe"; Tasks: desktopicon

[Registry]
; Same Run value VisKey64.exe writes itself ("Tự khởi động VisKey cùng hệ điều hành Windows").
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "VisKey"; ValueData: """{app}\VisKey64.exe"""; Tasks: startup
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "VisKey"; Flags: uninsdeletevalue dontcreatekey
; Keep the in-app checkbox consistent with the startup task.
Root: HKCU; Subkey: "Software\VisKey"; ValueType: dword; ValueName: "LaunchAtStartup"; ValueData: "1"; Tasks: startup
Root: HKCU; Subkey: "Software\VisKey"; Flags: uninsdeletekeyifempty

[Run]
Filename: "{app}\VisKey64.exe"; Description: "{cm:LaunchProgram,VisKey}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{sys}\taskkill.exe"; Parameters: "/IM VisKey64.exe /F"; Flags: runhidden; RunOnceId: "StopVisKey"
