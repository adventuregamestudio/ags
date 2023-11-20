
#define AgsName "Adventure Game Studio"
#define AgsUrl "https://www.adventuregamestudio.co.uk/"
#define VcRedistInstaller "vc_redist.x86.exe"
#define VcRedistName "Microsoft Visual C++ 2015 Redistributable (x86)"

; requires following macros to be passed by command line:
;   AgsAppId - a GUID identifying installed software
;   AgsFullVersion - a 4-digit version number
;   AgsFriendlyVersion - a 3-digit 'user-friendly' version number
;   AgsSpVersion - special version tag (optional, can be empty)

#if "" == AgsSpVersion
#define AgsVerNameStr AgsName + ' ' + AgsFriendlyVersion
#define AgsOutputFile 'AGS-' + AgsFullVersion
#else
#define AgsVerNameStr AgsName + ' ' + AgsFriendlyVersion + ' ' + AgsSpVersion
#define AgsOutputFile 'AGS-' + AgsFullVersion + '-' + AgsSpVersion
#endif 

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={#AgsAppId}
AppName={#AgsName} {#AgsFriendlyVersion}
AppVersion={#AgsFullVersion}
AppVerName={#AgsVerNameStr}
AppPublisher=AGS Project Team
AppPublisherURL={#AgsUrl}
AppSupportURL={#AgsUrl}
AppUpdatesURL={#AgsUrl}
DefaultDirName={commonpf32}\{#AgsName} {#AgsFriendlyVersion}
DefaultGroupName={#AgsVerNameStr}
AllowNoIcons=yes
LicenseFile=License.txt
OutputBaseFilename={#AgsOutputFile}
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\AGSEditor.exe
ShowComponentSizes=yes


[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"


[CustomMessages]
ComponentMain=Main files
ComponentEngines=Engines
ComponentEngineDefault=Runtime engine for MS Windows
ComponentLinuxBuild=Linux build component
ComponentWebBuild=Web build component
ComponentAndroidBuild=Android build component
; ComponentDemoGame=Demo Game
InstallOptions=Install options
InstallVCRedist=Install {#VcRedistName}
CreateDesktopIcon=Create a &desktop icon
AssociateFiles=Associate AGF files with the editor


[Components]
Name: "main"; Description: "{cm:ComponentMain}"; Types: full compact custom; Flags: fixed
Name: "engine"; Description: "{cm:ComponentEngines}"; Types: full compact custom; Flags: fixed
Name: "engine\default"; Description: "{cm:ComponentEngineDefault}"; Types: full compact; Flags: exclusive
Name: "linux"; Description: "{cm:ComponentLinuxBuild}"; Types: full custom
Name: "web"; Description: "{cm:ComponentWebBuild}"; Types: full custom
Name: "android"; Description: "{cm:ComponentAndroidBuild}"; Types: full custom
; Name: "demogame"; Description: "{cm:ComponentDemoGame}"; Types: full custom


[Tasks]
Name: "vcredist"; Description: "{cm:InstallVCRedist}"; GroupDescription: "{cm:InstallOptions}"; Check: NOT VCRedistInstalled;
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:InstallOptions}"
Name: "associate"; Description: "{cm:AssociateFiles}"; GroupDescription: "{cm:InstallOptions}"


[Dirs]
Name: "{app}\Templates";


[Files]
; Engine files
Source: "Source\Engine\acwin.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine\default
Source: "Source\Engine\SDL2.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: engine\default
; Editor files
Source: "Source\Editor\AGSEditor.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\acsprset.spr"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\AGS.Controls.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\AGS.CScript.Compiler.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\AGS.Native.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\AGS.Types.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\AGSEditor.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\AGSEditor.exe.config"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\ikpMP3.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\ikpFlac.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\irrKlang.NET4.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\Magick.NET-Q8-x86.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\Magick.NET-Q8-x86.Native.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\Newtonsoft.Json.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\WeifenLuo.WinFormsUI.Docking.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Editor\WeifenLuo.WinFormsUI.Docking.ThemeVS2015.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: main
; Documentation
Source: "Source\Docs\ags-help.chm"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Docs\Changes.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: main
; Licenses
Source: "Source\Licenses\*"; DestDir: "{app}\Licenses"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
; URLs
Source: "Source\URLs\*"; DestDir: "{app}\URLs"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
; Templates
Source: "Source\Templates\*"; DestDir: "{app}\Templates"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
; Linux build components
Source: "Source\Linux\ags32"; DestDir: "{app}\Linux"; Flags: ignoreversion; Components: linux
Source: "Source\Linux\ags64"; DestDir: "{app}\Linux"; Flags: ignoreversion; Components: linux
Source: "Source\Linux\lib32\*"; DestDir: "{app}\Linux\lib32"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: linux
Source: "Source\Linux\lib64\*"; DestDir: "{app}\Linux\lib64"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: linux
Source: "Source\Linux\licenses\*"; DestDir: "{app}\Linux\licenses"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: linux
; Web build components
Source: "Source\Web\ags.wasm"; DestDir: "{app}\Web"; Flags: ignoreversion; Components: web
Source: "Source\Web\ags.js"; DestDir: "{app}\Web"; Flags: ignoreversion; Components: web
Source: "Source\Web\index.html"; DestDir: "{app}\Web"; Flags: ignoreversion; Components: web
; Android build components
Source: "Source\Android\mygame\*"; DestDir: "{app}\Android\mygame"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: android
Source: "Source\Android\gradle\*"; DestDir: "{app}\Android\gradle"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: android
Source: "Source\Android\library\*"; DestDir: "{app}\Android\library"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: android
Source: "Source\Android\plugins\*"; DestDir: "{app}\Android\plugins"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: android
; Demo game
; Source: "Source\Demo Game\*"; DestDir: "{code:GetDemoGameDir}"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist; Components: demogame
; Visual C++ runtime
Source: "Source\Redist\{#VcRedistInstaller}"; DestDir: {tmp}; Flags: deleteafterinstall; Tasks: vcredist
; NOTE: Don't use "Flags: ignoreversion" on any shared system files


[Icons]
Name: "{group}\AGS Editor {#AgsFriendlyVersion}"; Filename: "{app}\AGSEditor.exe"; Comment: "What are you waiting for? Fire it up and start making the best game ever!";
; Name: "{group}\Demo Game"; Filename: "{code:GetDemoGameDir}\game.agf"; Comment: "Here's one we made earlier! If you want a sneak peak at a working game, check it out."; Components: demogame
Name: "{group}\AGS Manual"; Filename: "{app}\ags-help.chm"; Comment: "Online help, tutorials and reference. THIS IS YOUR BIBLE NOW!"
Name: "{group}\{cm:UninstallProgram,Adventure Game Studio}"; Filename: "{uninstallexe}"; Comment: ":~(  Ah well, nothing lasts forever. Turn off the light on your way out."
Name: "{group}\Visit the AGS Website"; Filename: "{app}\URLs\AGS Website.url"; Comment: "See the latest AGS-related news. Find games to play."
Name: "{group}\Visit the AGS Forums"; Filename: "{app}\URLs\AGS Forums.url"; Comment: "Join the madness! Come on down and party on the forums."
Name: "{commondesktop}\AGS {#AgsFriendlyVersion}"; Filename: "{app}\AGSEditor.exe"; Tasks: desktopicon


[Registry]
Root: HKCR; Subkey: ".agf"; ValueType: string; ValueName: ""; ValueData: "AGSGameSource"; Flags: uninsdeletevalue; Tasks: associate
Root: HKCR; Subkey: "AGSGameSource"; ValueType: string; ValueName: ""; ValueData: "AGS Game"; Flags: uninsdeletekey; Tasks: associate
Root: HKCR; Subkey: "AGSGameSource\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\AGSEditor.exe,0"; Tasks: associate
Root: HKCR; Subkey: "AGSGameSource\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\AGSEditor.exe"" ""%1"""; Tasks: associate


[Run]
Filename: "{tmp}\{#VcRedistInstaller}"; StatusMsg: "Installing {#VcRedistName}..."; Parameters: "/quiet /norestart"; Flags: skipifdoesntexist; Tasks: vcredist
Filename: "{app}\AGSEditor.exe"; Description: "{cm:LaunchProgram,Adventure Game Studio}"; Flags: nowait postinstall skipifsilent;


[Code]

// Letting user choose destination for the Demo Game

var
  DemoGameDirPage: TInputDirWizardPage;

function GetDemoGameDir(Param: String): String;
begin
  Result := DemoGameDirPage.Values[0];
end;

procedure InitializeWizard();
begin
  // create a directory input page
  DemoGameDirPage := CreateInputDirPage(wpSelectComponents, 'Select Demo Game installation folder', 'Where should the Demo Game be installed', 'Demo Game will be installed in the following folder.'#13#10#13#10 +
    'To continue, click Next. If you would like to select a different folder, click Browse.', True, 'Demo Game');
  // add directory input page items
  DemoGameDirPage.Add('');
  // assign default directories for the items from the previously stored data; if
  // there are no data stored from the previous installation, use default folders
  // of your choice
  DemoGameDirPage.Values[0] := GetPreviousData('DemoGamePath', ExpandConstant('{commondocs}/Demo Game'));
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  case PageID of
  // skip demo game's install path page, if the component was not selected
  DemoGameDirPage.ID: Result := not WizardIsComponentSelected('demogame');
  else Result := False;
  end;
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  // store chosen directories for the next run of the setup
  SetPreviousData(PreviousDataKey, 'DemoGamePath', DemoGameDirPage.Values[0]);
end;


[Code]
const
  // Platform check
  PLATFORM_CHECK_ERROR_MESSAGE = 'This program is only supported on Windows Vista or newer.';

  // Visual C++ runtime
  // https://download.microsoft.com/download/6/A/A/6AA4EDFF-645B-48C5-81CC-ED5963AEAD48/vc_redist.x86.exe
  VCPP_REDIST_MAJOR_VERSION = 14.0;
  VCPP_REDIST_BUILD_VERSION = 24215;

  // .NET Framework 4.6 or newer
  // in theory this is only needed for OS versions older than Windows 10
  // https://docs.microsoft.com/en-us/dotnet/framework/migration-guide/how-to-determine-which-versions-are-installed#minimum-version
  DOT_NET_46_RELEASE_VERSION = 393295;
  NEED_DOT_NET_ERROR_MESSAGE = 'AGS needs the Microsoft .NET Framework 4.6 or later to be installed on this computer. Press OK to visit the Microsoft website and download this, then run Setup again.';
  DOT_NET_INSTALL_URL = 'https://dotnet.microsoft.com/download/dotnet-framework';

function VCRedistInstalled: Boolean;
var
  bld: Cardinal;
begin
  Result := (RegQueryDWordValue(
    HKLM,
    Format('SOFTWARE\Microsoft\VisualStudio\%.1f\VC\Runtimes\X86', [VCPP_REDIST_MAJOR_VERSION]),
    'Bld',
    bld)) AND (bld >= VCPP_REDIST_BUILD_VERSION);
end;

function DotNet46Installed: Boolean;
var
  version: Cardinal;
begin
  Result := (RegQueryDWordValue(
    HKLM,
    'Software\Microsoft\NET Framework Setup\NDP\v4\Full',
    'Release',
    version)) AND (version >= DOT_NET_46_RELEASE_VERSION);
end;

function IsWindowsVersionOrNewer(Major, Minor: Integer): Boolean;
var
  Version: TWindowsVersion;
begin
  GetWindowsVersionEx(Version);
  Result :=
    (Version.Major > Major) OR
    ((Version.Major = Major) AND (Version.Minor >= Minor));
end;

function IsWindowsVistaOrNewer: Boolean;
begin
  Result := IsWindowsVersionOrNewer(6, 0);
end;

function InitializeSetup(): Boolean;
var
  ErrorCode: Integer;
begin
  if (NOT IsWindowsVistaOrNewer) AND (NOT WizardSilent) then
  begin
    MsgBox(PLATFORM_CHECK_ERROR_MESSAGE, mbCriticalError, MB_OK);
  end;
  if NOT DotNet46Installed then
  begin
    if NOT WizardSilent then
    begin
      MsgBox(NEED_DOT_NET_ERROR_MESSAGE, mbInformation, MB_OK);
      ShellExecAsOriginalUser('', DOT_NET_INSTALL_URL, '', '', SW_SHOW, ewNoWait, ErrorCode);
    end;
    Result := False;
  end
  else begin
    Result := True;
  end;
end;
