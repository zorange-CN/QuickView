; QuickView Inno Setup Script
; Version: 1.1.0
; Author: justnullname

#define MyAppName "QuickView"
#ifndef ExePath
  #define ExePath "..\Release\" + AppArch + "\QuickView.exe"
#endif

#ifndef MyAppVersion
  #if FileExists(ExePath)
    #define MyAppVersion GetFileVersion(ExePath)
  #else
    #define MyAppVersion "2.1.0"
  #endif
#endif
#define MyAppPublisher "justnullname"
#define MyAppURL "https://github.com/justnullname/QuickView"
#define MyAppExeName "QuickView.exe"

; Architectures: x64, arm64
#ifndef AppArch
  #define AppArch "x64"
#endif

[Setup]
AppId={{D175135F-5C7A-4E7B-B41E-C638E75C421A}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\LICENSE
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=..\Release\installer
#ifdef OutputName
  OutputBaseFilename={#OutputName}
#else
  OutputBaseFilename=QuickView_Installer_{#AppArch}
#endif
SetupIconFile=..\QuickView.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma
SolidCompression=yes
WizardStyle=modern

#if AppArch == "x64"
  ArchitecturesAllowed=x64
  ArchitecturesInstallIn64BitMode=x64
#elif AppArch == "arm64"
  ArchitecturesAllowed=arm64
  ArchitecturesInstallIn64BitMode=arm64
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce

[Files]
; Assume build artifacts are organized in Release\<Arch>\ directory
Source: "{#ExePath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "--uninstall"; Flags: runhidden

[UninstallDelete]
Type: files; Name: "{app}\QuickView.ini"
Type: files; Name: "{app}\*.old"
Type: filesandordirs; Name: "{userappdata}\QuickView"
