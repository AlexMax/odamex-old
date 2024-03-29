; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Odamex
AppVerName=Odamex 0.7.0
AppPublisher=Odamex Dev Team
AppPublisherURL=http://odamex.net
AppSupportURL=http://odamex.net
AppUpdatesURL=http://odamex.net
DefaultDirName={userpf}\odamex
DefaultGroupName=Odamex
AllowNoIcons=true
LicenseFile=..\..\LICENSE
;InfoBeforeFile=..\..\CHANGES
OutputBaseFilename=odamex-win32-0.7.0
Compression=lzma2
SolidCompression=true
VersionInfoProductName=Odamex Windows Installer
VersionInfoProductVersion=0.7.0
AlwaysShowDirOnReadyPage=true
ChangesEnvironment=true
AppID={{2E517BBB-916F-4AB6-80E0-D4A292513F7A}
;PrivilegesRequired=none
PrivilegesRequired=none
ShowLanguageDialog=auto
UninstallDisplayIcon={app}\odamex.exe
VersionInfoCompany=Odamex
AppVersion=0.7.0
EnableDirDoesntExistWarning=true
DirExistsWarning=no
VersionInfoVersion=0.7.0
MinVersion=0,5.0
AllowRootDirectory=True
ChangesAssociations=Yes
ArchitecturesInstallIn64BitMode=x64
UninstallDisplaySize=23068672
UsePreviousAppDir = yes
;AppModifyPath={app}\UninsHs.exe /m0=Odamex
WizardImageFile=..\..\media\wininstall_largeback.bmp
WizardSmallImageFile=C:\odamex\trunk\media\wininstall_wizardicon.bmp

[Languages]
Name: english; MessagesFile: compiler:Default.isl
Name: french; MessagesFile: compiler:Languages\French.isl
Name: german; MessagesFile: compiler:Languages\German.isl
Name: spanish; MessagesFile: compiler:Languages\Spanish.isl
Name: en; MessagesFile: compiler:Default.isl
Name: fr; MessagesFile: compiler:Languages\French.isl
Name: de; MessagesFile: compiler:Languages\German.isl
Name: es; MessagesFile: compiler:Languages\Spanish.isl

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked

[Types]
Name: full; Description: Full installation
Name: compact; Description: Client-only installation
Name: custom; Description: Custom installation; Flags: iscustom

[Components]
Name: base; Description: Base data; Types: full compact custom; Flags: fixed
Name: client; Description: Odamex Client; Types: full compact custom; Flags: DisableNoUninstallWarning
Name: server; Description: Odamex Server; Types: full; Flags: DisableNoUninstallWarning
Name: launcher; Description: Odalaunch (Game Launcher); Types: full compact custom; Flags: DisableNoUninstallWarning
Name: libs; Description: Libraries (SDL 1.2.15, SDL_Mixer 1.2.12); Types: full compact; Flags: DisableNoUninstallWarning


[Files]
; NOTE: Don't use "Flags: ignoreversion" on any shared system files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 64-BIT FILES
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Source: ..\..\m64\odamex.exe; DestDir: {app}; Flags: ignoreversion; Components: client; Check: Is64BitInstallMode
Source: ..\..\m64\odasrv.exe; DestDir: {app}; Flags: ignoreversion; Components: server; Check: Is64BitInstallMode
;Source: ..\..\m64\odalaunch.exe; DestDir: {app}; Flags: ignoreversion; Components: launcher; Check: Is64BitInstallMode
Source: ..\..\m64\SDL.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: Is64BitInstallMode
Source: ..\..\m64\SDL_mixer.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: Is64BitInstallMode
Source: ..\..\m64\libogg-0.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: Is64BitInstallMode
Source: ..\..\m64\smpeg.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: Is64BitInstallMode
Source: ..\..\m64\libvorbis-0.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: Is64BitInstallMode
Source: ..\..\m64\libvorbisfile-3.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: Is64BitInstallMode
Source: ..\..\m64\libmikmod-2.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: Is64BitInstallMode

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 32-BIT FILES
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Source: ..\..\odamex.exe; DestDir: {app}; Flags: ignoreversion; Components: client; Check: not Is64BitInstallMode
Source: ..\..\odasrv.exe; DestDir: {app}; Flags: ignoreversion; Components: server; Check: not Is64BitInstallMode
;Source: ..\..\odalaunch.exe; DestDir: {app}; Flags: ignoreversion; Components: launcher; Check: not Is64BitInstallMode
Source: ..\..\SDL.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: not Is64BitInstallMode
Source: ..\..\SDL_mixer.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: not Is64BitInstallMode
Source: ..\..\libogg-0.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: not Is64BitInstallMode
Source: ..\..\smpeg.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: not Is64BitInstallMode
Source: ..\..\libvorbis-0.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: not Is64BitInstallMode
Source: ..\..\libvorbisfile-3.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: not Is64BitInstallMode
Source: ..\..\libmikmod-2.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: not Is64BitInstallMode
;Source: ..\..\libwinpthread-1.dll; DestDir: {app}; Flags: ignoreversion; Components: libs; Check: not Is64BitInstallMode

Source: ..\..\odalaunch.exe; DestDir: {app}; Flags: ignoreversion; Components: launcher
Source: ..\..\config-samples\*; DestDir: {app}\config-samples; Flags: ignoreversion; Components: server
Source: ..\..\odamex.wad; DestDir: {app}; Flags: ignoreversion; Components: client server
;Source: ..\..\COPYING.winpthreads.txt; DestDir: {app}; Flags: ignoreversion; Components: libs
Source: ..\..\CHANGELOG; DestDir: {app}; Flags: ignoreversion; Components: base
Source: ..\..\LICENSE; DestDir: {app}; Flags: ignoreversion; Components: base
Source: ..\..\MAINTAINERS; DestDir: {app}; Flags: ignoreversion; Components: base
; Source: "UninsHs.exe"; DestDir: "{app}"; Flags: restartreplace

[Dirs]
;Name: "{localappdata}\odamex"; Flags: uninsalwaysuninstall

[INI]
Filename: {app}\Odamex Website.url; Section: InternetShortcut; Key: URL; String: http://odamex.net
Filename: {app}\Releases Changelog.url; Section: InternetShortcut; Key: URL; String: http://odamex.net/wiki/Releases

[Icons]
Name: {group}\Odamex Client; Filename: {app}\odamex.exe; WorkingDir: {app}
Name: {group}\Odalaunch; Filename: {app}\odalaunch.exe; WorkingDir: {app}
Name: {group}\Odamex Server; Filename: {app}\odasrv.exe; WorkingDir: {app}
Name: {group}\{cm:ProgramOnTheWeb,Odamex}; Filename: {app}\Odamex Website.url
Name: {group}\Releases Changelog; Filename: {app}\Releases Changelog.url
Name: {group}\{cm:UninstallProgram,Odamex}; Filename: {uninstallexe}
;Name: "{group}\{cm:UninstallProgram,Odamex}"; Filename: "{app}\UninsHs.exe"; Parameters: /u3=Odamex; WorkingDir: {app}
Name: {userdesktop}\Odamex Launcher; Filename: {app}\odalaunch.exe; Tasks: desktopicon; WorkingDir: {app}; IconIndex: 0; Components: launcher

[Run]
Filename: {app}\odalaunch.exe; Description: {cm:LaunchProgram,Odalaunch}; Flags: nowait postinstall skipifsilent
;Filename: {app}\UninsHs.exe; Parameters: /r={{2E517BBB-916F-4AB6-80E0-D4A292513F7A},{language},{srcexe}; Flags: runminimized
;Filename: {app}\UninsHs.exe; Parameters: /r=Odamex,{language},{srcexe},{localappdata}\odamex\setup.exe
;Flags: nowait runhidden runminimized


[UninstallDelete]
Type: files; Name: {app}\Odamex Website.url
Type: files; Name: {app}\Releases Changelog.url
Type: files; Name: {app}\odamex.out
Type: files; Name: {app}\odamex.cfg
Type: files; Name: {app}\odasrv.cfg
Type: files; Name: {app}\*.log
;Type: filesandordirs; Name: "{localappdata}\odamex"
Type: dirifempty; Name: "{app}"

[Registry]
Root: HKCR; Subkey: odamex; ValueType: string; ValueData: URL:Odamex Protocol; Flags: uninsdeletekey noerror
Root: HKCR; Subkey: odamex; ValueType: string; ValueName: Url Protocol; Flags: createvalueifdoesntexist uninsdeletekey noerror
Root: HKCR; Subkey: odamex\DefaultIcon; ValueData: odamex.exe,1; Flags: createvalueifdoesntexist uninsdeletekey noerror
Root: HKCR; Subkey: odamex\shell\open\command; ValueData: """{app}\odamex.exe"" ""%1"""; Flags: createvalueifdoesntexist uninsdeletekey noerror; ValueType: string
Root: HKCR; SubKey: ".odd"; ValueType: string; ValueData: "Odamex Data Demo"; Flags: uninsdeletekey
Root: HKCR; SubKey: "Odamex Data Demo"; ValueType: string; ValueData: "Odamex Game Demo Format"; Flags: uninsdeletekey
Root: HKCR; SubKey: "Odamex Data Demo\Shell\Open\Command"; ValueType: string; ValueData: """{app}\odamex.exe"" ""%1"""; Flags: uninsdeletekey
Root: HKCR; Subkey: "Odamex Data Demo\DefaultIcon"; ValueType: string; ValueData: "{app}\odamex.exe,1"; Flags: uninsdeletevalue

[Code]
function ShouldSkipPage(CurPage: Integer): Boolean;
begin
  if Pos('/SP-', UpperCase(GetCmdTail)) > 0 then
    case CurPage of
      wpLicense, wpPassword, wpInfoBefore, wpUserInfo,
      wpSelectDir, wpSelectProgramGroup, wpInfoAfter:
        Result := True;
    end;
end;

const
  WM_LBUTTONDOWN = 513;
  WM_LBUTTONUP = 514;

procedure InitializeWizard();
begin
  if (Pos('/SP-', UpperCase(GetCmdTail)) > 0) then
  begin
    PostMessage(WizardForm.NextButton.Handle,WM_LBUTTONDOWN,0,0);
    PostMessage(WizardForm.NextButton.Handle,WM_LBUTTONUP,0,0);
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if (Pos('/SP-', UpperCase(GetCmdTail)) > 0) and
    (CurPageID = wpSelectComponents) then
    WizardForm.BackButton.Visible := False;
end;
