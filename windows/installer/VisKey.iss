; SPDX-License-Identifier: GPL-3.0-or-later
;
; Inno Setup script for VisKey for Windows (by 1VISION) — based on OpenKey by Mai Vũ Tuyên.
; Optional: the portable zip is the primary download. Built by .github/workflows/windows.yml:
;   iscc /DAppVersion=1.1.0 /DSourceExe=build\Release\VisKey.exe /DRepoRoot=. /Odist windows\installer\VisKey.iss
;
; Installs per user by default (no admin prompt) into %LOCALAPPDATA%\Programs\VisKey; the user may pick
; "all users" (Program Files) in the dialog. Settings then live next to VisKey.exe when that folder is
; writable (per-user install), otherwise in %APPDATA%\VisKey (see windows/README.md).

#ifndef AppVersion
  #define AppVersion "1.1.0"
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
AppCopyright=GPL-3.0-or-later. Based on OpenKey by Mai Vũ Tuyên.
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
UninstallDisplayIcon={app}\VisKey.exe
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
Source: "{#RepoRoot}\NOTICE"; DestDir: "{app}"; DestName: "NOTICE.txt"; Flags: ignoreversion
Source: "{#RepoRoot}\windows\README-Windows.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]
Name: "{autoprograms}\VisKey"; Filename: "{app}\VisKey.exe"
Name: "{autodesktop}\VisKey"; Filename: "{app}\VisKey.exe"; Tasks: desktopicon

[Registry]
; Same value VisKey writes itself from its menu ("Khởi động cùng Windows").
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "VisKey"; ValueData: """{app}\VisKey.exe"""; Tasks: startup
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "VisKey"; Flags: uninsdeletevalue dontcreatekey

[Run]
Filename: "{app}\VisKey.exe"; Description: "{cm:LaunchProgram,VisKey}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{sys}\taskkill.exe"; Parameters: "/IM VisKey.exe /F"; Flags: runhidden; RunOnceId: "StopVisKey"
