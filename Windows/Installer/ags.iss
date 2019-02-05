
#define AgsName "Adventure Game Studio"
#define AgsUrl "http://www.adventuregamestudio.co.uk/"
#define VcRedistInstaller "vcredist_x86-9.0.30729.6161.exe"
; requires following macros to be passed by command line:
;   AgsVersion - 4 digit version number
;   AgsFriendlyVersion - 3 digit 'user-friendly' version number
;   AgsSpVersion - special version tag (can be empty)

#if "" == AgsSpVersion
#define AgsVerNameStr AgsName + ' ' + AgsFriendlyVersion
#define AgsOutputFile 'AGS-' + AgsFriendlyVersion
#else
#define AgsVerNameStr AgsName + ' ' + AgsFriendlyVersion + ' ' + AgsSpVersion
#define AgsOutputFile 'AGS-' + AgsFriendlyVersion + '-' + AgsSpVersion
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
DefaultDirName={pf}\{#AgsName} {#AgsFriendlyVersion}
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
ComponentDemoGame=Demo Game
InstallOptions=Install options
InstallVCRedist=Install Visual C++ Redistributable 2008 SP1
CreateDesktopIcon=Create a &desktop icon
AssociateFiles=Associate AGF files with the editor


[Components]
Name: "main"; Description: "{cm:ComponentMain}"; Types: full compact custom; Flags: fixed
Name: "engine"; Description: "{cm:ComponentEngines}"; Types: full compact custom; Flags: fixed
Name: "engine\default"; Description: "{cm:ComponentEngineDefault}"; Types: full compact; Flags: exclusive
Name: "linux"; Description: "{cm:ComponentLinuxBuild}"; Types: full custom
Name: "demogame"; Description: "{cm:ComponentDemoGame}"; Types: full custom


[Tasks]
Name: "vcredist"; Description: "{cm:InstallVCRedist}"; GroupDescription: "{cm:InstallOptions}"; Check: VCRedistNeedsInstall;
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:InstallOptions}"
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:InstallOptions}"; Flags: unchecked;
Name: "associate"; Description: "{cm:AssociateFiles}"; GroupDescription: "{cm:InstallOptions}"


[Dirs]
Name: "{app}\Templates";


[Files]
Source: "Source\engine\acwin.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine\default
Source: "Source\AGSEditor.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\Linux\*"; DestDir: "{app}\Linux"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist; Components: linux
Source: "Source\ags-help.chm"; DestDir: "{app}"; Flags: ignoreversion; Components: main
Source: "Source\*"; DestDir: "{app}"; Excludes: "*.pdb"; Flags: ignoreversion; Components: main
Source: "Source\Docs\*"; DestDir: "{app}\Docs"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
Source: "Source\Templates\*"; DestDir: "{app}\Templates"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: main
Source: "Source\Demo Game\*"; DestDir: "{code:GetDemoGameDir}"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: demogame
Source: "{#VcRedistInstaller}"; DestDir: {tmp}; Flags: deleteafterinstall; Tasks: vcredist
; NOTE: Don't use "Flags: ignoreversion" on any shared system files


[Icons]
Name: "{group}\AGS Editor"; Filename: "{app}\AGSEditor.exe"; Comment: "What are you waiting for? Fire it up and start making the best game ever!";
Name: "{group}\Demo Game"; Filename: "{code:GetDemoGameDir}\game.agf"; Comment: "Here's one we made earlier! If you want a sneak peak at a working game, check it out."; Components: demogame
Name: "{group}\AGS Manual"; Filename: "{app}\ags-help.chm"; Comment: "Online help, tutorials and reference. THIS IS YOUR BIBLE NOW!"
Name: "{group}\{cm:UninstallProgram,Adventure Game Studio}"; Filename: "{uninstallexe}"; Comment: ":~(  Ah well, nothing lasts forever. Turn off the light on your way out."
Name: "{group}\Visit the AGS Website"; Filename: "{app}\Docs\AGS Website.url"; Comment: "See the latest AGS-related news. Find games to play."
Name: "{group}\Visit the AGS Forums"; Filename: "{app}\Docs\AGS Forums.url"; Comment: "Join the madness! Come on down and party on the forums."
Name: "{commondesktop}\AGS {#AgsFriendlyVersion}"; Filename: "{app}\AGSEditor.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\AGS {#AgsFriendlyVersion}"; Filename: "{app}\AGSEditor.exe"; Tasks: quicklaunchicon


[Registry]
Root: HKCR; Subkey: ".agf"; ValueType: string; ValueName: ""; ValueData: "AGSGameSource"; Flags: uninsdeletevalue; Tasks: associate
Root: HKCR; Subkey: "AGSGameSource"; ValueType: string; ValueName: ""; ValueData: "AGS Game"; Flags: uninsdeletekey; Tasks: associate
Root: HKCR; Subkey: "AGSGameSource\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\AGSEditor.exe,0"; Tasks: associate
Root: HKCR; Subkey: "AGSGameSource\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\AGSEditor.exe"" ""%1"""; Tasks: associate


[Run]
; "How to perform a silent install of the Visual C++ 2008 redistributable packages"
;   http://blogs.msdn.com/b/astebner/archive/2010/10/18/9513328.aspx
Filename: "{tmp}\{#VcRedistInstaller}"; Parameters: "/qb"; Flags: skipifdoesntexist; Tasks: vcredist
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
  DemoGameDirPage.ID: Result := not IsComponentSelected('demogame');
  else Result := False;
  end;
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  // store chosen directories for the next run of the setup
  SetPreviousData(PreviousDataKey, 'DemoGamePath', DemoGameDirPage.Values[0]);
end;


[Code]

// Based on code from "How to make vcredist_x86 reinstall only if not yet installed?"
//  http://stackoverflow.com/questions/11137424/how-to-make-vcredist-x86-reinstall-only-if-not-yet-installed
// "How to detect the presence of the Visual C++ 9.0 runtime redistributable package"
//  http://blogs.msdn.com/b/astebner/archive/2009/01/29/9384143.aspx

#IFDEF UNICODE
  #DEFINE AW "W"
#ELSE
  #DEFINE AW "A"
#ENDIF

type
  INSTALLSTATE = Longint;

const
  INSTALLSTATE_INVALIDARG = -2;  // An invalid parameter was passed to the function.
  INSTALLSTATE_UNKNOWN = -1;     // The product is neither advertised or installed.
  INSTALLSTATE_ADVERTISED = 1;   // The product is advertised but not installed.
  INSTALLSTATE_ABSENT = 2;       // The product is installed for a different user.
  INSTALLSTATE_DEFAULT = 5;      // The product is installed for the current user.

  VC_2008_SP1_MFC_SEC_UPD_REDIST_X86 = '{9BE518E6-ECC6-35A9-88E4-87755C07200F}';
  VC_2008_SP1_MFC_SEC_UPD_REDIST_X64 = '{5FCE6D76-F5DC-37AB-B2B8-22AB8CEDB1D4}';
  VC_2008_SP1_MFC_SEC_UPD_REDIST_IA64 = '{515643D1-4E9E-342F-A75A-D1F16448DC04}';

  DOT_NET_REGISTRY_KEY = 'Software\Microsoft\.NETFramework\policy\v2.0';

  // NEED_DOT_NET_ERROR_MESSAGE = 'AGS needs the Microsoft .NET Framework 2.0 or later to be installed on this computer. Press OK to visit the Microsoft website and download this, then run Setup again.';
  NEED_DOT_NET_ERROR_MESSAGE = 'AGS needs the Microsoft .NET Framework 2.0 or later to be installed on this computer. Enable Microsoft .NET Framework in Windows Features then run Setup again. Press OK to visit the Microsoft website with instructions.';

  // DOT_NET_INSTALL_URL = 'http://www.microsoft.com/downloads/details.aspx?FamilyID=0856EACB-4362-4B0D-8EDD-AAB15C5E04F5&displaylang=en';
  DOT_NET_INSTALL_URL = 'http://windows.microsoft.com/en-us/windows/turn-windows-features-on-off';

var
  ErrorCode: Integer;

function MsiQueryProductState(szProduct: string): INSTALLSTATE;
  external 'MsiQueryProductState{#AW}@msi.dll stdcall';

function VCVersionInstalled(const ProductID: string): Boolean;
begin
  Result := MsiQueryProductState(ProductID) = INSTALLSTATE_DEFAULT;
end;

function VCRedistNeedsInstall: Boolean;
begin
  // Here the Result must be True when you need to install your VCRedist
  // or False when you don't need to.
  // The following won't install your VC redist only when the Visual C++
  // 2008 SP1 Redist (x86) is installed for the current user
  Result := not VCVersionInstalled(VC_2008_SP1_MFC_SEC_UPD_REDIST_X86);
end;


function InitializeSetup(): Boolean;
begin
  if not RegKeyExists(HKLM, DOT_NET_REGISTRY_KEY) then
  begin
    MsgBox(NEED_DOT_NET_ERROR_MESSAGE, mbInformation, MB_OK);
    ShellExec('', DOT_NET_INSTALL_URL, '', '', SW_SHOW, ewNoWait, ErrorCode);
    Result := False;
  end
  else begin
    Result := True;
  end;
end;
