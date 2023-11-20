using System;
using System.Collections.Generic;
using System.Configuration;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.Threading;
using System.Threading.Tasks;
using AGS.CScript.Compiler;
using AGS.Types;
using AGS.Types.Interfaces;
using AGS.Editor.Preferences;
using AGS.Editor.Utils;
using System.Net;

namespace AGS.Editor
{
    public class AGSEditor : IAGSEditorDirectories
    {
        public event GetScriptHeaderListHandler GetScriptHeaderList;
        public event GetScriptModuleListHandler GetScriptModuleList;
        public delegate void PreCompileGameHandler(PreCompileGameEventArgs evArgs);
        public event PreCompileGameHandler PreCompileGame;
        public delegate void AttemptToSaveGameHandler(ref bool allowSave);
        public event AttemptToSaveGameHandler AttemptToSaveGame;
        public delegate void PreSaveGameHandler(PreSaveGameEventArgs evArgs);
        public event PreSaveGameHandler PreSaveGame;
		public delegate void PreDeleteSpriteHandler(PreDeleteSpriteEventArgs evArgs);
		public event PreDeleteSpriteHandler PreDeleteSprite;
		public delegate void ProcessAllGameTextsHandler(IGameTextProcessor processor, CompileMessages errors);
        public event ProcessAllGameTextsHandler ProcessAllGameTexts;
        public delegate void ExtraCompilationStepHandler(CompileMessages errors);
        public event ExtraCompilationStepHandler ExtraCompilationStep;
        public delegate void ExtraOutputCreationStepHandler(bool miniExeForDebug);
        public event ExtraOutputCreationStepHandler ExtraOutputCreationStep;

		public const string BUILT_IN_HEADER_FILE_NAME = "_BuiltInScriptHeader.ash";
        public const string OUTPUT_DIRECTORY = "Compiled";
        public const string DATA_OUTPUT_DIRECTORY = "Data"; // subfolder in OUTPUT_DIRECTORY for data file outputs
        public const string DEBUG_OUTPUT_DIRECTORY = "_Debug";
        public const string GAME_FILE_NAME = "Game.agf";
		public const string BACKUP_EXTENSION = "bak";
        public const string OLD_GAME_FILE_NAME = "ac2game.dta";
        public const string TEMPLATES_DIRECTORY_NAME = "Templates";
        public const string AGS_REGISTRY_KEY = @"SOFTWARE\Adventure Game Studio\AGS Editor";
        public const string SPRITE_FILE_NAME = "acsprset.spr";
        public const string SPRITE_INDEX_FILE_NAME = "sprindex.dat";

        /* 
         * LATEST_XML_VERSION is the last version of the Editor that used 4-point-4-number string
         * to identify the version of AGS that saved game project.
         * DO NOT MODIFY THIS CONSTANT UNLESS YOU REALLY WANT TO CHANGE THE IDENTIFICATION METHOD.
        */
        public const string LATEST_XML_VERSION = "3.0.3.2";

        /*
         * LATEST_XML_VERSION_INDEX is the current project XML version.
         * DO increase this number for every new public release that introduces a new
         * property in the main project's XML, otherwise people who are trying to open
         * newer projects in older Editors will get confusing error messages, instead
         * of clear "wrong version of AGS" message.
        */
        /*
         *  6: 3.2.1
         *  7: 3.2.2
         *  8: 3.3.1.1163 - Settings.LastBuildConfiguration;
         *  9: 3.4.0.1    - Settings.CustomResolution
         * 10: 3.4.0.2    - Settings.BuildTargets
         * 11: 3.4.0.4    - Region.TintLuminance
         * 12: 3.4.0.8    - Settings.UseOldCustomDialogOptionsAPI & ScriptAPILevel
         * 13: 3.4.0.9    - Settings.ScriptCompatLevel
         * 14: 3.4.1      - Settings.RenderAtScreenResolution
         * 15: 3.4.1.2    - DefaultSetup node
         *     3.4.3      - Added missing audio properties to DefaultSetup [ forgot to change version index!! ]
         * 16: 3.5.0      - Unlimited fonts (need separate version to prevent crashes in older editors)
         * 17: 3.5.0.4    - Extended sprite source properties
         * 18: 3.5.0.8    - Disallow relative asset resolutions by default, added flag for compatibility;
         *                  Real sprite resolution; Individual font scaling; Default room mask resolution
         * 19: 3.5.0.11   - Custom Say and Narrate functions for dialog scripts. GameFileName.
         * 20: 3.5.0.14   - Sprite.ImportAlphaChannel.
         * 21: 3.5.0.15   - AudioClip ID.
         * 22: 3.5.0.18   - Settings.ScaleMovementSpeedWithMaskResolution.
         * 23-24: 3.5.0.20+ - Sprite tile import properties.
         * 25: 3.5.0.22   - Full editor version saved into XML header, RuntimeSetup.ThreadedAudio.
         * 26:            - Fixed sound references in game properties.
         * 27: 3.5.1      - Settings.AttachDataToExe
         * --------------------------------------------------------------------
         * Since 3.6.0 define format value as AGS version represented as NN,NN,NN,NN:
         * e.g. 3.6.0     is 03,06,00,00 (3060000),
         *      4.12.3.25 is 04,12,03,25 (4120325), and so on
         * --------------------------------------------------------------------
         * 3.6.0          - Settings.CustomDataDir, TTFHeightDefinedBy, TTFMetricsFixup;
         *                - Font.AutoOutlineStyle, AutoOutlineThickness;
         *                - Character.IdleDelay, Character.IdleAnimationDelay;
         *                - Cursor.AnimationDelay
         *                - Room.BackgroundAnimationEnabled
         *                - RuntimeSetup.FullscreenDesktop
         * 3.6.0.20       - Settings.GameTextEncoding, Settings.UseOldKeyboardHandling;
         * 3.6.1.2        - GUIListBox.Translated property moved to GUIControl parent
         * 3.6.1.3        - RuntimeSetup.TextureCache, SoundCache
         * 3.6.1.9        - Settings.ScaleCharacterSpriteOffsets
         * 3.6.1.10       - SetRestartPoint() is no longer auto called in the engine,
         *                  add one into the global script when importing older games.
         * 
         * 3.99.99.00     - BlendMode for various objects, Character.Transparency.
         * 3.99.99.01     - Open rooms
         * 3.99.99.07     - PO translations
         * 4.00.00.00     - Raised for org purposes without project changes
         *
        */
        public const int    LATEST_XML_VERSION_INDEX = 4000000;
        /// <summary>
        /// XML version index on the release of AGS 4.0.0, this constant be used to determine
        /// if upgrade of Rooms/Sprites/etc. to new format have been performed.
        /// </summary>
        public const int    AGS_4_0_0_XML_VERSION_INDEX_OPEN_ROOMS = 3999901;
        public const int    AGS_4_0_0_XML_VERSION_INDEX_PO_TRANSLATIONS = 3999907;
        /*
         * LATEST_USER_DATA_VERSION is the last version of the user data file that used a
         * 4-point-4-number string to identify the version of AGS that saved the file.
         * DO NOT MODIFY THIS CONSTANT UNLESS YOU REALLY WANT TO CHANGE THE IDENTIFICATION METHOD.
        */
        private const string LATEST_USER_DATA_VERSION = "3.0.2.1";
        /*
         * 1: 3.0.2.1
         * 2: 3.4.0.1    - WorkspaceState section
         * 3: 3.5.0.11
         * 4: 3.5.1 (?)
        */
        public const int LATEST_USER_DATA_XML_VERSION_INDEX = 4;
        public const string AUDIO_VOX_FILE_NAME = "audio.vox";

        private const string USER_DATA_FILE_NAME = GAME_FILE_NAME + USER_DATA_FILE_SUFFIX;
        private const string USER_DATA_FILE_SUFFIX = ".user";
        private const string LOCK_FILE_NAME = "_OpenInEditor.lock";
        private const string XML_USER_DATA_ROOT_NODE_NAME = "AGSEditorUserData";
        private const string XML_ROOT_NODE_NAME = "AGSEditorDocument";
        private const string XML_ATTRIBUTE_VERSION = "Version";
        public const string XML_ATTRIBUTE_VERSION_INDEX = "VersionIndex";
        public const string XML_ATTRIBUTE_EDITOR_VERSION = "EditorVersion";
        public const string COMPILED_DTA_FILE_NAME = "game28.dta";
        public const string CONFIG_FILE_NAME = "acsetup.cfg";
        public const string ENGINE_EXE_FILE_NAME = "acwin.exe";
        public const string CUSTOM_ICON_FILE_NAME = "user.ico";
        public const string SETUP_ICON_FILE_NAME = "setup.ico";
        public const string SETUP_PROGRAM_SOURCE_FILE = "setup.dat";
        public const string COMPILED_SETUP_FILE_NAME = "winsetup.exe";

        public readonly string[] RestrictedGameDirectories = new string[]
        {
            OUTPUT_DIRECTORY, DEBUG_OUTPUT_DIRECTORY, AudioClip.AUDIO_CACHE_DIRECTORY,
            Components.SpeechComponent.SPEECH_DIRECTORY
        };

        private Game _game;
        private string _editorExePath;
        private Script _builtInScriptHeader;
        private Script _autoGeneratedHeader;
        private AppSettings _applicationSettings = null;
        private Tasks _tasks = new Tasks();
        private IEngineCommunication _engineComms = new NamedPipesEngineCommunication();
        private DebugController _debugger;
		private bool _applicationStarted = false;
        private FileStream _lockFile = null;

        private static readonly IDictionary<ScriptAPIVersion, string> _scriptAPIVersionMacros =
            new SortedDictionary<ScriptAPIVersion, string>();
        private static readonly IDictionary<ScriptAPIVersion, string> _scriptCompatLevelMacros =
            new SortedDictionary<ScriptAPIVersion, string>();

        private static AGSEditor _instance;

        public static AGSEditor Instance
        {
            get
            {
                if (_instance == null)
                {
                    _instance = new AGSEditor();
                }
                return _instance;
            }
        }

        private AGSEditor()
        {
            _editorExePath = Process.GetCurrentProcess().MainModule.FileName;

            CreateAppSettings();
        }

        static AGSEditor()
        {
            foreach (ScriptAPIVersion v in Enum.GetValues(typeof(ScriptAPIVersion)))
            {
                if (v == ScriptAPIVersion.Highest)
                    continue; // don't enlist "Highest" constant
                _scriptAPIVersionMacros[v] = "SCRIPT_API_" + v.ToString();
            }
            foreach (ScriptAPIVersion v in Enum.GetValues(typeof(ScriptAPIVersion)))
            {
                if (v == ScriptAPIVersion.Highest)
                    continue; // don't enlist "Highest" constant
                _scriptCompatLevelMacros[v] = "SCRIPT_COMPAT_" + v.ToString();
            }
            BuildTargetsInfo.RegisterBuildTarget(new BuildTargetDataFile());
            BuildTargetsInfo.RegisterBuildTarget(new BuildTargetWindows());
            BuildTargetsInfo.RegisterBuildTarget(new BuildTargetDebug());
            BuildTargetsInfo.RegisterBuildTarget(new BuildTargetLinux());
            BuildTargetsInfo.RegisterBuildTarget(new BuildTargetWeb());
            BuildTargetsInfo.RegisterBuildTarget(new BuildTargetAndroid());
        }

        public Game CurrentGame
        {
            get { return _game; }
        }

        public Tasks Tasks
        {
            get { return _tasks; }
        }

        public string EditorDirectory
        {
            get { return Path.GetDirectoryName(_editorExePath); }
        }

        public string TemplatesDirectory
        {
            get { return Path.Combine(this.EditorDirectory, TEMPLATES_DIRECTORY_NAME); }
        }

        public string GameDirectory
        {
            get { return _game.DirectoryPath; }
        }

        public string LocalAppData
        {
            get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "AGS"); }
        }

        public string UserTemplatesDirectory
        {
            get { return Path.Combine(LocalAppData, TEMPLATES_DIRECTORY_NAME); }
        }

        public string BaseGameFileName
        {
            get
            {
                return string.IsNullOrWhiteSpace(_game.Settings.GameFileName) ?
                    Path.GetFileName(this.GameDirectory) : _game.Settings.GameFileName;
            }
        }

        public Script BuiltInScriptHeader
        {
            get { return _builtInScriptHeader; }
        }

        public Script AutoGeneratedHeader
        {
            get
            {
                if (_autoGeneratedHeader == null)
                {
                    RegenerateScriptHeader(null);
                }
                return _autoGeneratedHeader;
            }
        }

        public AppSettings Settings
        {
            get { return _applicationSettings; }
        }

        public DebugController Debugger
        {
            get { return _debugger; }
        }

		public bool ApplicationStarted
		{
            get { return _applicationStarted; }
			set { _applicationStarted = value; }
		}

        public void DoEditorInitialization()
        {
            // disable SSL v3
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;

            try
            {
                Directory.CreateDirectory(UserTemplatesDirectory);
            }
            catch
            {
                // this is an optional folder that might have user data in
                // the parent folder, so don't try too hard to force this
            }

            _game = new Game();
            _debugger = new DebugController(_engineComms);
            _debugger.BreakAtLocation += new DebugController.BreakAtLocationHandler(_debugger_BreakAtLocation);

            _builtInScriptHeader = new Script(BUILT_IN_HEADER_FILE_NAME, Resources.ResourceManager.GetResourceAsString("agsdefns.sh"), true);
            AutoComplete.ConstructCache(_builtInScriptHeader, null);

            CompileMessages errors = new CompileMessages();
            Factory.NativeProxy.NewGameLoaded(Factory.AGSEditor.CurrentGame, errors);
            ReportGameLoad(errors);
        }

        public void ReportGameLoad(CompileMessages errors)
        {
            if (errors.Count == 1)
            {
                Factory.GUIController.ShowMessage(errors[0].Message, MessageBoxIcon.Warning);
            }
            else if (errors.Count > 1)
            {
                Factory.GUIController.ShowOutputPanel(errors);
                Factory.GUIController.ShowMessage("Game was loaded, but there were errors or warnings.", MessageBoxIcon.Warning);
            }
        }

        public void Dispose()
        {
            CloseLockFile();
            _debugger.EditorShutdown();
        }

        private void CreateAppSettings()
        {
            // Test for a valid config file, in case of corrupt config
            // delete the file and notify the user that the prefs will be reset.
            AppSettings appSettings;
            string configFileName = null;
            try
            {
                var cfg = ConfigurationManager.OpenExeConfiguration(ConfigurationUserLevel.PerUserRoamingAndLocal);
                configFileName = cfg.FilePath;
                appSettings = new AppSettings();
            }
            catch (ConfigurationErrorsException ex)
            {
                string filename = !string.IsNullOrEmpty(ex.Filename) ? ex.Filename : configFileName;
                Factory.GUIController.ShowMessage("Editor's configuration is corrupt and cannot be loaded. " +
                        "This could happen because of an improper program exit, a disk malfunction, or an invalid file edit. " +
                        "The configuration file will be deleted and user preferences reset to allow Editor start." +
                        "\n\n\nConfiguration file's location: " + (!string.IsNullOrEmpty(filename) ? configFileName : "(undefined)"),
                        MessageBoxIcon.Error);
                Utilities.TryDeleteFile(filename);
                appSettings = new AppSettings();
            }

            _applicationSettings = appSettings;
        }

        private void _debugger_BreakAtLocation(DebugCallStack callStack)
        {
            Factory.GUIController.HideOutputPanel();
            Factory.GUIController.ShowCallStack(callStack);
            Factory.GUIController.ZoomToFile(callStack.Lines[0].ScriptName, callStack.Lines[0].LineNumber, true, callStack.ErrorMessage);
        }

        public void RegenerateScriptHeader(Room currentRoom)
        {
            _autoGeneratedHeader = _tasks.RegenerateScriptHeader(_game, currentRoom);
            AutoComplete.ConstructCache(_autoGeneratedHeader, GetAllScriptHeaders());
        }

        public string GetFirstAvailableScriptName(string namePrefix, int startIndex = 1, Room room = null)
        {
            int tryIndex = startIndex;
            string tryName = null;
            bool canUseName = false;
            while (!canUseName)
            {
                tryName = namePrefix + tryIndex;
                canUseName = !_game.IsScriptNameAlreadyUsed(tryName, null);
                if (canUseName && room != null)
                    canUseName = !room.IsScriptNameAlreadyUsed(tryName, null);
                if (!canUseName)
                {
                    tryIndex++;
                }
            }
            return tryName;
        }

        /// <summary>
        /// Gets a list of all built-in and plugin headers that the
        /// user cannot change
        /// </summary>
        private List<Script> GetInternalScriptHeaders()
        {
            List<Script> scripts = new List<Script>();
            scripts.Add(_builtInScriptHeader);
            scripts.Add(this.AutoGeneratedHeader);

            if (GetScriptHeaderList != null)
            {
                GetScriptHeaderList(new GetScriptHeaderListEventArgs(scripts));
            }

            return scripts;
        }

        /// <summary>
        /// Gets a list of all built-in and plugin modules that the
        /// user cannot change
        /// </summary>
        private List<Script> GetInternalScriptModules()
        {
            List<Script> scripts = new List<Script>();

            if (GetScriptModuleList != null)
            {
                GetScriptModuleList(new GetScriptModuleListEventArgs(scripts));
            }

            return scripts;
        }

        public List<IScript> GetAllScripts()
        {
            return GetAllScripts(true);
        }

        public List<IScript> GetAllScripts(bool includeDialogs)
        {
            List<IScript> scripts = new List<IScript>();
            foreach (Script script in _game.RootScriptFolder.AllScriptsFlat)
            {
                scripts.Add(script);
            }
            foreach (IRoom room in _game.RootRoomFolder.AllItemsFlat)
            {
                if (room.Script == null)
                {
                    room.LoadScript();
                    scripts.Add(room.Script);
                    room.UnloadScript();
                }
                else scripts.Add(room.Script); 
            }
            if (includeDialogs)
            {
                foreach (Dialog dialog in _game.RootDialogFolder.AllItemsFlat)
                {
                    scripts.Add(dialog);
                }
            }
            return scripts;
        }

        public IList<Script> GetAllScriptHeaders()
        {
            List<Script> scripts = GetInternalScriptHeaders();

            foreach (ScriptAndHeader script in _game.RootScriptFolder.AllItemsFlat)
            {
                scripts.Add(script.Header);
            }
            return scripts;
        }

        /// <summary>
        /// Finds a script header corresponding to this script.
        /// If the given script is a header itself, then returns it.
        /// Returns null if this script does not have a header pair.
        /// </summary>
        public Script GetScriptHeaderFor(Script script)
        {
            if (script.IsHeader) return script;
            foreach (ScriptAndHeader sh in _game.RootScriptFolder.AllItemsFlat)
            {
                if (sh.Script.UniqueKey == script.UniqueKey)
                    return sh.Header;
            }
            return null;
        }

        /// <summary>
        /// Gets all script headers that are supposed to be imported into the given
        /// script: according to the current AGS rules these are all headers preceding
        /// this script in the list. Includes the script's own header, if available.
        /// </summary>
        public List<Script> GetImportedScriptHeaders(Script script, bool onlyInternal = false)
        {
            Script header = GetScriptHeaderFor(script);
            int hkey = header != null ? header.UniqueKey : -1;
            var allHeaders = onlyInternal ? GetInternalScriptHeaders() : GetAllScriptHeaders();
            var list = allHeaders.TakeWhile(s => s.UniqueKey != hkey).ToList();
            if (header != null)
                list.Add(header);
            return list;
        }

		public void DeleteFileOnDisk(string fileName)
		{
			DeleteFileOnDisk(new string[] { fileName });
		}

		public void DeleteFileOnDisk(string[] fileNames)
		{
			string[] fullPathNames = new string[fileNames.Length];
			for (int i = 0; i < fileNames.Length; i++)
			{
				fullPathNames[i] = Path.GetFullPath(fileNames[i]);
			}

			foreach (string fileName in fullPathNames)
			{
                Utilities.TryDeleteFile(fileName);
			}
		}

        public void RenameFileOnDisk(string currentName, string newName)
        {
            string sourcePath = Path.GetFullPath(currentName);
            string destPath = Path.GetFullPath(newName);
            File.Move(sourcePath, destPath);
        }

        /// <summary>
        /// Attempt to get write access to the specified file. If this fails,
        /// a dialog will be displayed to the user and false will be returned.
        /// </summary>
        public bool AttemptToGetWriteAccess(string fileName)
        {
            List<string> fileNames = new List<string>();
            fileNames.Add(fileName);
            return AttemptToGetWriteAccess(fileNames);
        }

        public bool AttemptToGetWriteAccess(IList<string> fileNames)
        {
            List<string> fileNamesWithFullPaths = new List<string>();
            foreach (string fileName in fileNames)
            {
				if (File.Exists(fileName))
				{
					fileNamesWithFullPaths.Add(Path.GetFullPath(fileName));
				}
            }

            bool success = true;
            foreach (string fileName in fileNamesWithFullPaths)
            {
                success = success & CheckFileSystemWriteAccess(fileName);
            }
            return success;
        }

        private bool CheckFileSystemWriteAccess(string fileName)
        {
            if (!File.Exists(fileName))
            {
                try
                {
                    StreamWriter sw = new StreamWriter(fileName);
                    sw.Close();
					Utilities.TryDeleteFile(fileName);
                }
                catch (Exception ex)
                {
                    Factory.GUIController.ShowMessage("Unable to create the file '" + fileName + "' due to an error: " + ex.Message, MessageBoxIcon.Warning);
                    return false;
                }
                // File does not exist, but we do have permission to create it
                return true;
            }

            if ((File.GetAttributes(fileName) & FileAttributes.ReadOnly) != 0)
            {
                Factory.GUIController.ShowMessage("Unable to edit file '" + fileName + "' because it is read-only.", MessageBoxIcon.Warning);
                return false;
            }

            return true;
        }

        private void LoadUserDataFile(string fileName)
        {
            XmlNode docNode = null;
            try
            {
                XmlDocument doc = new XmlDocument();
                doc.Load(fileName);
                if (doc.DocumentElement.Name != XML_USER_DATA_ROOT_NODE_NAME)
                {
                    throw new AGSEditorException("Invalid user data file. This is not a valid AGS user data file.");
                }
                string fileVersion = doc.DocumentElement.Attributes[XML_ATTRIBUTE_VERSION].InnerXml;
                if (fileVersion != LATEST_USER_DATA_VERSION)
                {
                    throw new AGSEditorException("User data file is from a newer version of AGS or an unsupported beta version. Please check the AGS website for a newer version of the editor.");
                }
                string userDataSavedWithEditorVersion = null;
                XmlAttribute editorVersionNode = doc.DocumentElement.Attributes[XML_ATTRIBUTE_EDITOR_VERSION];
                if (editorVersionNode != null)
                {
                    userDataSavedWithEditorVersion = editorVersionNode.InnerText;
                }
                int? versionIndex = null;
                XmlAttribute versionIndexNode = doc.DocumentElement.Attributes[XML_ATTRIBUTE_VERSION_INDEX];
                if (versionIndexNode != null)
                {
                    // From 3.0.2.1 we switched to a simple integer version, it's easier to
                    // compare than using the 4-point version
                    versionIndex = Convert.ToInt32(versionIndexNode.InnerText);
                    if ((versionIndex < 1) || (versionIndex > LATEST_USER_DATA_XML_VERSION_INDEX))
                    {
                        throw new AGSEditorException("This game's user data file is from " +
                            ((userDataSavedWithEditorVersion == null) ? "a newer version" : ("version " + userDataSavedWithEditorVersion))
                            + " of AGS or an unsupported beta version. Please check the AGS website for a newer version of the editor.");
                    }
                }
                docNode = doc.DocumentElement;
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("Unable to read the user preferences file. You may lose some of your Preferences settings and may not be able to access Source Control." + Environment.NewLine + Environment.NewLine + "The error was: " + ex.Message, MessageBoxIcon.Warning);
            }

            Factory.Events.OnLoadedUserData(docNode);
            _game.WorkspaceState.FromXml(docNode);
        }

        private void VerifyGameNotAlreadyOpenInAnotherEditor()
        {
            try
            {
                try
                {
                    File.Delete(LOCK_FILE_NAME);
                }
                catch (UnauthorizedAccessException)
                {
                    File.SetAttributes(LOCK_FILE_NAME, FileAttributes.Normal);
                    File.Delete(LOCK_FILE_NAME);
                }
            }
            catch
            {
                throw new AGSEditorException("Cannot load the game, because it is already open in another copy of the AGS Editor");
            }
        }

        public void LoadGameFile(string fileName)
        {
            if (File.Exists(LOCK_FILE_NAME))
            {
                VerifyGameNotAlreadyOpenInAnotherEditor();
            }

            if (File.Exists(fileName + USER_DATA_FILE_SUFFIX))
            {
                LoadUserDataFile(fileName + USER_DATA_FILE_SUFFIX);
            }
            else
            {
                Factory.Events.OnLoadedUserData(null);
            }

            XmlDocument doc = new XmlDocument();
            doc.Load(fileName);
            if (doc.DocumentElement.Name != XML_ROOT_NODE_NAME)
            {
                throw new AGSEditorException("Invalid game data file. This is not a valid AGS game.");
            }
            string fileVersion = doc.DocumentElement.Attributes[XML_ATTRIBUTE_VERSION].InnerXml;
            if ((fileVersion != LATEST_XML_VERSION) &&
                (fileVersion != "3.0.1.1") &&
                (fileVersion != "3.0.2.1") &&
                (fileVersion != "3.0.2.2") &&
                (fileVersion != "3.0.3.1") && 
				(fileVersion != "2.80.1"))
            {
                throw new AGSEditorException("Game data file is from a newer version of AGS or an unsupported beta version. Please check the AGS website for a newer version of the editor.");
            }

            string gameSavedWithEditorVersion = null;
            XmlAttribute editorVersionNode = doc.DocumentElement.Attributes[XML_ATTRIBUTE_EDITOR_VERSION];
            if (editorVersionNode != null)
            {
                gameSavedWithEditorVersion = editorVersionNode.InnerText;
            }

            int? versionIndex = null;
            XmlAttribute versionIndexNode = doc.DocumentElement.Attributes[XML_ATTRIBUTE_VERSION_INDEX];
            if ((versionIndexNode == null) && (fileVersion == LATEST_XML_VERSION))
            {
                throw new AGSEditorException("Invalid format game data file");
            }
            else if (versionIndexNode != null)
            {
                // From 3.0.3 we switch to a simple integer version, it's easier to
                // compare than using the 4-point version
                versionIndex = Convert.ToInt32(versionIndexNode.InnerText);
                if (versionIndex == 5)
                {
                    throw new AGSEditorException("This game is from an unsupported beta version of AGS and cannot be loaded.");
                }
                if ((versionIndex < 1) || (versionIndex > LATEST_XML_VERSION_INDEX))
                {
                    throw new AGSEditorException("Game data file is from " +
                        ((gameSavedWithEditorVersion == null) ? "a newer version" : ("version " + gameSavedWithEditorVersion))
                        + " of AGS or an unsupported beta version. Please check the AGS website for a newer version of the editor.");
                }
            }

            _game.SavedXmlVersionIndex = versionIndex;
			_game.SavedXmlVersion = fileVersion;
            _game.SavedXmlEditorVersion = gameSavedWithEditorVersion;
            try
            { // Try to retrieve the xml encoding declaration, to be used as a fallback for older projects
                XmlDeclaration dec = doc.FirstChild as XmlDeclaration;
                _game.SavedXmlEncodingCodePage = Encoding.GetEncoding(dec.Encoding).CodePage;
            }
            catch (Exception) {}

			_game.FromXml(doc.DocumentElement);

            Factory.Events.OnGameLoad(doc.DocumentElement);
        }

        public void RefreshEditorAfterGameLoad(Game newGame, CompileMessages errors)
        {
            _game = newGame;

            Factory.GUIController.GameNameUpdated();
            Factory.NativeProxy.NewGameLoaded(Factory.AGSEditor.CurrentGame, errors);
            Factory.Events.OnRefreshAllComponentsFromGame();

            RegenerateScriptHeader(null);

            List<Script> importedScripts = GetInternalScriptHeaders();
            foreach (ScriptAndHeader script in newGame.RootScriptFolder.AllItemsFlat)
            {
                AutoComplete.ConstructCache(script.Header, importedScripts);
                importedScripts.Add(script.Header);
            }

			Factory.GUIController.ProjectTree.CollapseAll();

			if (_engineComms.SupportedOnCurrentSystem)
			{
				_engineComms.ResetWithCurrentPath();
			}

            CloseLockFile();

            try
            {
                _lockFile = File.Create(LOCK_FILE_NAME, 32, FileOptions.DeleteOnClose);
            }
            catch (UnauthorizedAccessException)
            {
                // If the whole game folder is read-only, don't worry about it
                _lockFile = null;
            }
        }

        private void CloseLockFile()
        {
            if (_lockFile != null)
            {
                _lockFile.Close();
                _lockFile = null;
            }
        }

		private void DefineMacrosAccordingToGameSettings(IPreprocessor preprocessor)
		{
			preprocessor.DefineMacro("AGS_NEW_STRINGS", "1");
			preprocessor.DefineMacro("AGS_SUPPORTS_IFVER", "1");
			if (_game.Settings.DebugMode)
			{
				preprocessor.DefineMacro("DEBUG", "1");
			}
            // Some settings have become obsolete and now have default values
            // EnforceObjectBasedScript => true
            preprocessor.DefineMacro("STRICT", "1");
            // EnforceNewStrings => true
            preprocessor.DefineMacro("STRICT_STRINGS", "1");
            // EnforceNewAudio => true
            preprocessor.DefineMacro("STRICT_AUDIO", "1");
            // UseOldCustomDialogOptionsAPI => false
            preprocessor.DefineMacro("NEW_DIALOGOPTS_API", "1");
            if (!_game.Settings.UseOldKeyboardHandling)
            {
                preprocessor.DefineMacro("NEW_KEYINPUT_API", "1");
            }
            // Define Script API level macros
            foreach (ScriptAPIVersion v in Enum.GetValues(typeof(ScriptAPIVersion)))
            {
                if (v == ScriptAPIVersion.Highest)
                    continue; // skip Highest constant
                if (v > _game.Settings.ScriptAPIVersionReal)
                    continue;
                preprocessor.DefineMacro(_scriptAPIVersionMacros[v], "1");
            }
            foreach (ScriptAPIVersion v in Enum.GetValues(typeof(ScriptAPIVersion)))
            {
                if (v == ScriptAPIVersion.Highest)
                    continue; // skip Highest constant
                if (v < _game.Settings.ScriptCompatLevelReal)
                    continue;
                preprocessor.DefineMacro(_scriptCompatLevelMacros[v], "1");
            }
        }

        private void DefineMacrosFromCompiler(IPreprocessor preprocessor)
        {
            var exts = Factory.NativeProxy.GetCompilerExtensions(_game);
            foreach (var ext in exts)
            {
                preprocessor.DefineMacro("SCRIPT_EXT_" + ext, "1");
            }
        }

        /// <summary>
        /// Preprocesses and then compiles the script using the supplied headers.
        /// Warnings and errors are collected in 'messages'.
        /// Will _not_ throw whenever compiling results in an error.
        /// </summary>
        public void CompileScript(Script script, List<Script> headers, CompileMessages messages)
		{
			messages = messages ?? new CompileMessages();
            List<string> preProcessedCode;
            CompileMessages preProcessingResults;
            PreprocessScript(script, headers, out preProcessedCode, out preProcessingResults);
            messages.AddRange(preProcessingResults);

            // If the preprocessor has found any errors then don't attempt compiling proper
            if (preProcessingResults.Count > 0)
                return;

            Factory.NativeProxy.CompileScript(script, preProcessedCode.ToArray(), _game, messages);
		}

        private void PreprocessScript(Script script, List<Script> headers, out List<string> preProcessedCode, out CompileMessages results)
        {
            IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
            DefineMacrosAccordingToGameSettings(preprocessor);
            DefineMacrosFromCompiler(preprocessor);

            preProcessedCode = new List<string>();
            foreach (Script header in headers)
                preProcessedCode.Add(preprocessor.Preprocess(header.Text, header.FileName));

            preProcessedCode.Add(preprocessor.Preprocess(script.Text, script.FileName));

            // Note that preProcessingResults is a list of CompilerMessage, 
            // which is completely different to  a list of CompileMessage.
            // So rewrite each CompilerMessage into a CompileMessage.
            // Currently, the preprocessor can only issue errors, no warnings,
            // so specifically, rewrite each CompilerMessage into a CompileError.
            results = new CompileMessages();
            foreach (CompilerMessage msg in preprocessor.Results)
                results.Add(new CompileError(msg.Message, msg.ScriptName, msg.LineNumber));
        }

        private Script CompileDialogs(CompileMessages errors, bool rebuildAll)
        {
            DialogScriptConverter dialogConverter = new DialogScriptConverter();
            string dialogScriptsText = dialogConverter.ConvertGameDialogScripts(_game, errors, rebuildAll);
            Script dialogScripts = new Script(Script.DIALOG_SCRIPTS_FILE_NAME, dialogScriptsText, false);

            // A dialog_request must exist in the global script, otherwise
            // the dialogs script fails to load at run-time
            // TODO: check if it's still true, and is necessary!
            Script script = CurrentGame.RootScriptFolder.GetScriptByFileName(Script.GLOBAL_SCRIPT_FILE_NAME, true);
            if (script != null)
            {
                script.Text = ScriptGeneration.InsertFunction(script.Text, "dialog_request", "int param");
            }

            return dialogScripts;
        }

        private struct CompileTask
        {
            public Script Script;
            public List<Script> Headers;
            public CompileTask(Script script, List<Script> headers)
            {
                Script = script;
                Headers = headers;
            }
        };

        private object CompileScripts(IWorkProgress progress, object parameter)
        {
            CompileScriptsParameters parameters = (CompileScriptsParameters)parameter;
            CompileMessages errors = parameters.Errors;
            CompileMessages messagesToReturn = new CompileMessages();
            RegenerateScriptHeader(null);
            List<Script> headers = GetInternalScriptHeaders();

            Script dialogScripts = CompileDialogs(errors, parameters.RebuildAll);

            // Collect the scripts that need to be compiled
            _game.ScriptsToCompile = new ScriptsAndHeaders();
            List<CompileTask> compileTasks = new List<CompileTask>();
            foreach (Script script in GetInternalScriptModules())
            {
                compileTasks.Add(new CompileTask(script, headers));
                _game.ScriptsToCompile.Add(new ScriptAndHeader(null, script));
            }
            foreach (ScriptAndHeader scripts in _game.RootScriptFolder.AllItemsFlat)
            {
                headers.Add(scripts.Header);
                compileTasks.Add(new CompileTask(scripts.Script, headers));
                _game.ScriptsToCompile.Add(scripts);
            }
            compileTasks.Add(new CompileTask(dialogScripts, headers));
            _game.ScriptsToCompile.Add(new ScriptAndHeader(null, dialogScripts));

            // Compile the scripts
            if (_game.Settings.ExtendedCompiler)
            {
                // The extended compiler doesn't use static memory, 
                // so several instances can run in parallel.
                Parallel.ForEach(compileTasks, (ct, state) =>
                {
                    CompileMessages messages = new CompileMessages();

                    CompileScript(ct.Script, ct.Headers, messages);
                    lock (messagesToReturn)
                    {
                        if (!messagesToReturn.HasErrors)
                            messagesToReturn.AddRange(messages);
                    }
                    if (messages.HasErrors)
                        state.Stop();
                });
            }
            else
            {
                foreach (CompileTask ct in compileTasks)
                {
                    CompileMessages messages = new CompileMessages();
                    CompileScript(ct.Script, ct.Headers, messages);
                    messagesToReturn.AddRange(messages);
                    if (messages.HasErrors)
                        break;
                }
            }

            // Copy the messages, but deduplicate the warnings
            // (duplicate warnings can show up if they are for a header
            // that is included in several source assemblies)
            {
                var alreadyIncluded = new HashSet<Tuple<string, int, string>>();
                foreach (CompileMessage mes in messagesToReturn)
                    if (mes is CompileError ||
                        alreadyIncluded.Add(Tuple.Create(mes.ScriptName, mes.LineNumber, mes.Message)))
                        errors.Add(mes);
            }
            ExtraCompilationStep?.Invoke(errors);
            
            return messagesToReturn;
        }

        private void CreateAudioVOXFile(bool forceRebuild)
        {
            List<string> fileListForVox = new List<string>();
            string audioVox = Path.Combine(OUTPUT_DIRECTORY, Path.Combine(DATA_OUTPUT_DIRECTORY, AUDIO_VOX_FILE_NAME));
            bool rebuildVox = (!File.Exists(audioVox)) || (forceRebuild);

            foreach (AudioClip clip in _game.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                if (clip.BundlingType == AudioFileBundlingType.InSeparateVOX)
                {
                    string thisFileName = clip.CacheFileName;
                    if (File.GetLastWriteTimeUtc(thisFileName) != clip.FileLastModifiedDate)
                    {
                        rebuildVox = true;
                        clip.FileLastModifiedDate = File.GetLastWriteTimeUtc(thisFileName);
                    }
                    fileListForVox.Add(thisFileName);
                }
            }

            if ((fileListForVox.Count == 0) || (rebuildVox))
            {
                Utilities.TryDeleteFile(audioVox);
            }

            if ((rebuildVox) && (fileListForVox.Count > 0))
            {
                DataFileWriter.MakeFlatDataFile(fileListForVox.ToArray(), 0, audioVox, false);
            }
        }

        private object CreateCompiledFiles(IWorkProgress progress, object parameter)
        {
            CompileScriptsParameters parameters = (CompileScriptsParameters)parameter;
            CompileMessages errors = parameters.Errors;
            bool forceRebuild = parameters.RebuildAll;

            // TODO: This is also awkward, we call Cleanup for active targets to make sure
            // that in case they changed the game binary name an old one gets removed.
            // Also please see the comment about build steps below.
            var buildNames = Factory.AGSEditor.CurrentGame.WorkspaceState.GetLastBuildGameFiles();
            foreach (IBuildTarget target in BuildTargetsInfo.GetSelectedBuildTargets())
            {
                // Primary cleanup
                target.DeleteMainGameData(Factory.AGSEditor.BaseGameFileName);

                // Old files cleanup (if necessary)
                string oldName;
                if (!buildNames.TryGetValue(target.Name, out oldName)) continue;
                if (!string.IsNullOrWhiteSpace(oldName) && oldName != Factory.AGSEditor.BaseGameFileName)
                    target.DeleteMainGameData(oldName);
            }

            IBuildTarget targetDataFile = BuildTargetsInfo.FindBuildTargetByName(BuildTargetsInfo.DATAFILE_TARGET_NAME);
            targetDataFile.Build(errors, forceRebuild); // ensure that data file is built first
            if (ExtraOutputCreationStep != null)
            {
                ExtraOutputCreationStep(false);
            }

            // TODO: As of now the build targets other than DataFile and Debug do DEPLOYMENT rather than BUILDING
            // (BuildTargetDebug, - which is never used right here, - seem to combine both operations:
            // building and preparing game to run under Windows).
            // This is why the BuildTargetDataFile is called explicitly at the start.
            // And that is why the rest must be called AFTER the ExtraOutputCreationStep.
            //
            // Possible solution that could improve situation could be to develop some kind of a BuildStep interface,
            // having BuildTargets providing their build steps of corresponding type and execution order.
            foreach (IBuildTarget target in BuildTargetsInfo.GetSelectedBuildTargets())
            {
                if (target != targetDataFile) target.Build(errors, forceRebuild);
                buildNames[target.Name] = Factory.AGSEditor.BaseGameFileName;
            }
            Factory.AGSEditor.CurrentGame.WorkspaceState.SetLastBuildGameFiles(buildNames);
            return null;
        }

		private void ReportErrorsIfAppropriate(CompileMessages errors)
		{
			if (errors.HasErrors)
			{
				if (_applicationSettings.MessageBoxOnCompile != MessageBoxOnCompile.Never)
				{
					Factory.GUIController.ShowMessage("There were compilation errors. See the output window for details.", MessageBoxIcon.Warning);
				}
			}
			else if (errors.Count > 0)
			{
				if (_applicationSettings.MessageBoxOnCompile != MessageBoxOnCompile.Never && _applicationSettings.MessageBoxOnCompile != MessageBoxOnCompile.OnlyErrors)
                {
                    Factory.GUIController.ShowMessage("There were warnings compiling your game. See the output window for details.", MessageBoxIcon.Warning);
				}
			}
		}

        private void RunPreCompilationChecks(CompileMessages errors)
        {
            if ((_game.LipSync.Type == LipSyncType.PamelaVoiceFiles) &&
                (_game.Settings.SpeechStyle == SpeechStyle.Lucasarts))
            {
                errors.Add(new CompileError("Voice lip-sync cannot be used with Lucasarts-style speech"));
            }

            if (_game.PlayerCharacter == null)
            {
                errors.Add(new CompileError("No character has been set as the player character"));
            }
            else if (_game.FindRoomByID(_game.PlayerCharacter.StartingRoom) == null)
            {
                errors.Add(new CompileError("The game is set to start in room " + _game.PlayerCharacter.StartingRoom + " which does not exist"));
            }

            if ((_game.Settings.ColorDepth == GameColorDepth.Palette) &&
                ((_game.DefaultSetup.GraphicsDriver == GraphicsDriver.D3D9) || (_game.DefaultSetup.GraphicsDriver == GraphicsDriver.OpenGL)))
            {
                errors.Add(new CompileWarning(
                    string.Format("{0} selected as a default graphics driver may not be well suited for a 256-colour game. Consider Software driver instead.",
                        Types.Utilities.GetEnumValueDescription(_game.DefaultSetup.GraphicsDriver))));
            }

			if ((_game.Settings.ColorDepth == GameColorDepth.Palette) &&
				(_game.Settings.RoomTransition == RoomTransitionStyle.CrossFade))
			{
				errors.Add(new CompileError("You cannot use the CrossFade room transition with 256-colour games"));
			}

			if ((_game.Settings.DialogOptionsGUI < 0) ||
				(_game.Settings.DialogOptionsGUI >= _game.RootGUIFolder.GetAllItemsCount()))
			{
				if (_game.Settings.DialogOptionsGUI != 0)
				{
					errors.Add(new CompileError("Invalid GUI number set for Dialog Options GUI"));
				}
			}

			foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
			{
				AGS.Types.View view = _game.FindViewByID(character.NormalView);
				if (view == null)
				{
					errors.Add(new CompileError("Character " + character.ID + " (" + character.RealName + ") has invalid normal view."));
				}
				else
				{
					EnsureViewHasAtLeast4LoopsAndAFrameInLeftRightLoops(view);
				}
			}

			Dictionary<string, AGS.Types.View> viewNames = new Dictionary<string, AGS.Types.View>();
			EnsureViewNamesAreUnique(_game.RootViewFolder, viewNames, errors);

            // Test if reserved audio channels do not exceed supported limit
            int reservedChans = 0;
            foreach (AudioClipType type in _game.AudioClipTypes)
            {
                if (type.MaxChannels <= 0) continue;
                reservedChans += type.MaxChannels;
                if (reservedChans > Game.MAX_USER_SOUND_CHANNELS)
                {
                    errors.Add(new CompileError("Audio type " + type.Name + " exceeds the limit of sound channels by " +
                        Math.Min(type.MaxChannels, reservedChans - Game.MAX_USER_SOUND_CHANNELS) +
                        ". The sum of reserved channels in ALL user audio types should not exceed " + Game.MAX_USER_SOUND_CHANNELS + "."));
                }
            }
            foreach (AudioClip clip in _game.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                if (!File.Exists(clip.CacheFileName))
                {
                    errors.Add(new CompileError("Audio file missing for " + clip.ScriptName + ": " + clip.CacheFileName));
                }
            }
        }

		private void EnsureViewHasAtLeast4LoopsAndAFrameInLeftRightLoops(AGS.Types.View view)
		{
			bool viewModified = false;
			while (view.Loops.Count < 4)
			{
				view.Loops.Add(new ViewLoop(view.Loops.Count));
				viewModified = true;
			}

			if (view.Loops[1].Frames.Count < 1)
			{
				view.Loops[1].Frames.Add(new ViewFrame(0));
			}
			if (view.Loops[2].Frames.Count < 1)
			{
				view.Loops[2].Frames.Add(new ViewFrame(0));
			}
			if (viewModified)
			{
				view.NotifyClientsOfUpdate();
			}
		}

		private void EnsureViewNamesAreUnique(ViewFolder folder, Dictionary<string, AGS.Types.View> viewNames, CompileMessages errors)
		{
			foreach (ViewFolder subFolder in folder.SubFolders)
			{
				EnsureViewNamesAreUnique(subFolder, viewNames, errors);
			}

			foreach (AGS.Types.View view in folder.Views)
			{
				if (!string.IsNullOrEmpty(view.Name))
				{
					if (viewNames.ContainsKey(view.Name.ToLower()))
					{
						errors.Add(new CompileError("There are two or more views with the same name '" + view.Name + "'"));
					}
					else
					{
						viewNames.Add(view.Name.ToLower(), view);
					}
				}
			}
		}

		public bool NeedsRebuildForDebugMode()
		{
			bool result;
			BuildConfiguration pending;

			pending = this._game.Settings.DebugMode ? BuildConfiguration.Debug : BuildConfiguration.Release;
			if (this._game.WorkspaceState.LastBuildConfiguration != pending)
			{
				result = true;
				this._game.WorkspaceState.LastBuildConfiguration = pending;
			}
			else
				result = false;

			return result;
		}

		public CompileMessages CompileGame(bool forceRebuild, bool createMiniExeForDebug)
        {
            Factory.GUIController.ClearOutputPanel();
            CompileMessages messages = new CompileMessages();

            Utilities.EnsureStandardSubFoldersExist();

            forceRebuild |= _game.WorkspaceState.RequiresRebuild;

            if (PreCompileGame != null)
            {
				PreCompileGameEventArgs evArgs = new PreCompileGameEventArgs(forceRebuild);
				evArgs.Errors = messages;

                PreCompileGame(evArgs);

                if (!evArgs.AllowCompilation)
                {
                    Factory.GUIController.ShowOutputPanel(messages);
					ReportErrorsIfAppropriate(messages);
                    return messages;
                }
            }

            RunPreCompilationChecks(messages);

			if (!messages.HasErrors)
				BusyDialog.Show(
                    "Please wait while your scripts are compiled...",
                    new BusyDialog.ProcessingHandler(CompileScripts),
                    new CompileScriptsParameters(messages, forceRebuild));

            if (!messages.HasErrors)
			{
				if (createMiniExeForDebug)
				{
					CreateMiniEXEForDebugging(messages);
				}
				else
				{
					CreateCompiledFiles(messages, forceRebuild);
				}
                _game.WorkspaceState.RequiresRebuild = false;
            }

            Factory.GUIController.ShowOutputPanel(messages);

			ReportErrorsIfAppropriate(messages);

            return messages;
        }

        /// <summary>
        /// Creates a mini-exe that only contains the GAME.DTA file,
        /// in order to improve compile speed.
        /// All other files will be sourced from the game folder.
        /// </summary>
        private void CreateMiniEXEForDebugging(CompileMessages errors)
        {
            IBuildTarget target = BuildTargetsInfo.FindBuildTargetByName(BuildTargetDebug.DEBUG_TARGET_NAME);

            var buildNames = Factory.AGSEditor.CurrentGame.WorkspaceState.GetLastBuildGameFiles();
            string oldName;
            if (buildNames.TryGetValue(target.Name, out oldName))
            {
                // Primary cleanup
                target.DeleteMainGameData(Factory.AGSEditor.BaseGameFileName);
                // Old files cleanup (if necessary)
                if (!string.IsNullOrWhiteSpace(oldName) && oldName != Factory.AGSEditor.BaseGameFileName)
                    target.DeleteMainGameData(oldName);
            }

            target.Build(errors, false);
            if (ExtraOutputCreationStep != null)
            {
                ExtraOutputCreationStep(true);
            }

            buildNames[target.Name] = Factory.AGSEditor.BaseGameFileName;
            Factory.AGSEditor.CurrentGame.WorkspaceState.SetLastBuildGameFiles(buildNames);
        }

        private void CreateCompiledFiles(CompileMessages errors, bool forceRebuild)
        {
            try
            {
                BusyDialog.Show("Please wait while your game is created...", new BusyDialog.ProcessingHandler(CreateCompiledFiles), new CompileScriptsParameters(errors, forceRebuild));
            }
            catch (Exception ex)
            {
                errors.Add(new CompileError("Unexpected error: " + ex.Message));
            }
        }

        private string[] ConstructFileListForEXE()
        {
            List<string> files = new List<string>();
            Utilities.AddAllMatchingFiles(files, "preload.pcx");
            Utilities.AddAllMatchingFiles(files, SPRITE_INDEX_FILE_NAME);
            foreach (AudioClip clip in _game.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                if (clip.BundlingType == AudioFileBundlingType.InGameEXE)
                {
                    files.Add(clip.CacheFileName);
                }
            }
            Utilities.AddAllMatchingFiles(files, "flic*.fl?");
            Utilities.AddAllMatchingFiles(files, COMPILED_DTA_FILE_NAME);
            Utilities.AddAllMatchingFiles(files, "agsfnt*.ttf");
            Utilities.AddAllMatchingFiles(files, "agsfnt*.wfn");
            Utilities.AddAllMatchingFiles(files, SPRITE_FILE_NAME);
            foreach (UnloadedRoom room in _game.RootRoomFolder.AllItemsFlat)
            {
                if (File.Exists(room.FileName))
                {
                    files.Add(room.FileName);
                }
            }
            Utilities.AddAllMatchingFiles(files, "*.ogv");
            return files.ToArray();
        }

		public bool AboutToDeleteSprite(int spriteNumber)
		{
			PreDeleteSpriteEventArgs evArgs = new PreDeleteSpriteEventArgs(spriteNumber);
			if (PreDeleteSprite != null)
			{
				PreDeleteSprite(evArgs);
			}
			return evArgs.AllowDelete;
		}

        public void DeleteSprite(Sprite sprite)
        {
            string usageReport = SpriteTools.GetSpriteUsageReport(sprite.Number, _game);
            if (usageReport != null)
            {
                throw new SpriteInUseException("Cannot delete a sprite because it is in use:" + Environment.NewLine + usageReport);
            }
            else if (AboutToDeleteSprite(sprite.Number))
            {
                SpriteFolder parent = _game.RootSpriteFolder.FindFolderThatContainsSprite(sprite.Number);
                if (parent == null)
                {
                    throw new AGSEditorException("The sprite " + sprite.Number + " could not be found in any sprite folders");
                }
                parent.Sprites.Remove(sprite);
                Factory.NativeProxy.DeleteSprite(sprite);
            }
            else
            {
                throw new SpriteInUseException("An editor component did not allow sprite " + sprite.Number + " to be deleted");
            }
        }

        public bool SaveGameFiles()
        {
            if (AttemptToSaveGame != null)
            {
                bool allowSave = true;
                AttemptToSaveGame(ref allowSave);
                if (!allowSave)
                {
                    return false;
                }
            }

			PreSaveGameEventArgs evArgs = new PreSaveGameEventArgs();
			if (PreSaveGame != null)
            {
                PreSaveGame(evArgs);
            }

            foreach (Script script in _game.RootScriptFolder.AllScriptsFlat)
            {
                if (script.Modified)
                {
                    if (AttemptToGetWriteAccess(script.FileName))
                    {
                        script.SaveToDisk();
                    }
                }
            }

            List<string> filesToCheck = new List<string>();
            filesToCheck.Add(USER_DATA_FILE_NAME);
            filesToCheck.Add(GAME_FILE_NAME);
            if (Factory.NativeProxy.AreSpritesModified)
            {
                filesToCheck.Add(SPRITE_FILE_NAME);
                filesToCheck.Add(SPRITE_INDEX_FILE_NAME);
            }

            if (!AttemptToGetWriteAccess(filesToCheck))
            {
                return false;
            }

            // Make sure the game's name in the Recent list is updated, in
            // case the user has just changed it
            RecentGame recentGame = new RecentGame(_game.Settings.GameName, _game.DirectoryPath);
            while (Settings.RecentGames.Contains(recentGame))
            {
                Settings.RecentGames.Remove(recentGame);
            }
            Settings.RecentGames.Insert(0, recentGame);
            Settings.Save();

            bool result;
            try
            {
                result = (bool)BusyDialog.Show("Please wait while your files are saved...", new BusyDialog.ProcessingHandler(SaveGameFilesProcess), null);
            }
            catch (Exception ex)
            { // CHECKME: rethrown exception from other thread duplicates original exception as inner one for some reason
                InteractiveTasks.ReportTaskException("An error occurred whilst trying to save your game.", ex.InnerException);
                result = false;
            }

			if (!evArgs.SaveSucceeded)
			{
				result = false;
			}
            return result;
        }

        private string CustomPathForConfig(bool use_custom_path, string custom_path)
        {
            string path_value = ""; // no value
            if (use_custom_path)
            {
                if (String.IsNullOrEmpty(custom_path))
                    path_value = "."; // same directory
                else
                    path_value = custom_path;
            }
            return path_value;
        }

        private static string GetGfxDriverConfigID(GraphicsDriver driver)
        {
            switch (driver)
            {
                case GraphicsDriver.Software: return "Software";
                case GraphicsDriver.D3D9: return "D3D9";
                case GraphicsDriver.OpenGL: return "OGL";
            }
            return "";
        }

        private static string MakeGameScalingConfig(GameScaling scaling)
        {
            switch (scaling)
            {
                case GameScaling.MaxInteger:
                case GameScaling.Integer:
                    return "round";
                case GameScaling.StretchToFit: return "stretch";
                case GameScaling.ProportionalStretch: return "proportional";
            }
            return "";
        }

        /// <summary>
        /// Writes the config file using particular game Settings and DefaultSetup options.
        /// </summary>
		public void WriteConfigFile(string configFilePath, bool resetFile = true)
		{
            if (resetFile)
                Utilities.TryDeleteFile(configFilePath);
            var sections = new Dictionary<string, Dictionary<string, string>>();
            sections.Add("graphics", new Dictionary<string, string>());
            sections.Add("language", new Dictionary<string, string>());
            sections.Add("misc", new Dictionary<string, string>());
            sections.Add("mouse", new Dictionary<string, string>());
            sections.Add("sound", new Dictionary<string, string>());
            sections.Add("touch", new Dictionary<string, string>());

            sections["graphics"]["driver"] = GetGfxDriverConfigID(_game.DefaultSetup.GraphicsDriver);
            sections["graphics"]["windowed"] = _game.DefaultSetup.Windowed ? "1" : "0";
            sections["graphics"]["fullscreen"] =
                _game.DefaultSetup.FullscreenDesktop ? "full_window" : "desktop";
            if (_game.DefaultSetup.GameScaling == GameScaling.Integer)
                sections["graphics"]["window"] =
                    String.Format("x{0}", _game.DefaultSetup.GameScalingMultiplier);
            else
                sections["graphics"]["window"] = "desktop";

            sections["graphics"]["game_scale_fs"] = MakeGameScalingConfig(_game.DefaultSetup.FullscreenGameScaling);
            sections["graphics"]["game_scale_win"] = MakeGameScalingConfig(_game.DefaultSetup.GameScaling);

            sections["graphics"]["filter"] = _game.DefaultSetup.GraphicsFilter;
            sections["graphics"]["vsync"] = _game.DefaultSetup.VSync ? "1" : "0";
            sections["graphics"]["antialias"] = _game.DefaultSetup.AAScaledSprites ? "1" : "0";
            bool render_at_screenres = _game.Settings.RenderAtScreenResolution == RenderAtScreenResolution.UserDefined ?
                _game.DefaultSetup.RenderAtScreenResolution : _game.Settings.RenderAtScreenResolution == RenderAtScreenResolution.True;
            sections["graphics"]["render_at_screenres"] = render_at_screenres ? "1" : "0";
            string[] rotation_str = new string[] { "unlocked", "portrait", "landscape" };
            sections["graphics"]["rotation"] = rotation_str[(int)_game.DefaultSetup.Rotation];

            bool audio_enabled = _game.DefaultSetup.DigitalSound != RuntimeAudioDriver.Disabled;
            sections["sound"]["enabled"] = audio_enabled ? "1" : "0";
            sections["sound"]["driver"] = ""; // always default
            sections["sound"]["usespeech"] = _game.DefaultSetup.UseVoicePack ? "1" : "0";

            sections["language"]["translation"] = _game.DefaultSetup.Translation;
            sections["mouse"]["auto_lock"] = _game.DefaultSetup.AutoLockMouse ? "1" : "0";
            sections["mouse"]["speed"] = _game.DefaultSetup.MouseSpeed.ToString(CultureInfo.InvariantCulture);

            // Touch input
            string[] emulate_mouse_str = new string[] { "off", "one_finger", "two_fingers" };
            sections["touch"]["emul_mouse_mode"] =
                emulate_mouse_str[(int)_game.DefaultSetup.TouchToMouseEmulation];
            sections["touch"]["emul_mouse_relative"] =
                ((int)_game.DefaultSetup.TouchToMouseMotionMode).ToString();

            // Note: the cache sizes are written in KB (while we have it in MB on the editor pane)
            sections["graphics"]["sprite_cache_size"] = (_game.DefaultSetup.SpriteCacheSize * 1024).ToString();
            sections["graphics"]["texture_cache_size"] = (_game.DefaultSetup.TextureCacheSize * 1024).ToString();
            sections["sound"]["cache_size"] = (_game.DefaultSetup.SoundCacheSize * 1024).ToString();

            sections["misc"]["user_data_dir"] = CustomPathForConfig(_game.DefaultSetup.UseCustomSavePath, _game.DefaultSetup.CustomSavePath);
            sections["misc"]["shared_data_dir"] = CustomPathForConfig(_game.DefaultSetup.UseCustomAppDataPath, _game.DefaultSetup.CustomAppDataPath);
            sections["misc"]["titletext"] = _game.DefaultSetup.TitleText;

            if(_game.Settings.DebugMode) // Do not write show_fps in a release build, this is only intended for the developer
            {
                sections["misc"]["show_fps"] = _game.DefaultSetup.ShowFPS ? "1" : "0";
            }

            NativeProxy.Instance.WriteIniFile(configFilePath, sections, true);
        }

        public void SaveUserDataFile()
        {
            StringWriter sw = new StringWriter();
            XmlTextWriter writer = new XmlTextWriter(sw);
            writer.Formatting = Formatting.Indented;
            writer.WriteProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"");
            writer.WriteComment("DO NOT EDIT THIS FILE. It is automatically generated by the AGS Editor, changing it manually could break your game");
            writer.WriteStartElement(XML_USER_DATA_ROOT_NODE_NAME);
            writer.WriteAttributeString(XML_ATTRIBUTE_VERSION, LATEST_USER_DATA_VERSION);
            writer.WriteAttributeString(XML_ATTRIBUTE_VERSION_INDEX, LATEST_USER_DATA_XML_VERSION_INDEX.ToString());
            writer.WriteAttributeString(XML_ATTRIBUTE_EDITOR_VERSION, AGS.Types.Version.AGS_EDITOR_VERSION);

            Factory.Events.OnSavingUserData(writer);

            _game.WorkspaceState.ToXml(writer);

            writer.WriteEndElement();
            writer.Flush();

            try
            {
                StreamWriter fileOutput = new StreamWriter(USER_DATA_FILE_NAME, false, Types.Utilities.UTF8);
                fileOutput.Write(sw.ToString());
                fileOutput.Close();
                writer.Close();
            }
            catch (UnauthorizedAccessException ex)
            {
                Factory.GUIController.ShowMessage("Unable to write the user data file. Ensure that you have write access to the game folder, and that the file is not already open.\n\n" + ex.Message, MessageBoxIcon.Warning);
            }
            catch (IOException ex)
            {
                Factory.GUIController.ShowMessage("Unable to write the user data file. Ensure that you have write access to the game folder, and that the file is not already open.\n\n" + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private object SaveGameFilesProcess(IWorkProgress progress, object parameter)
        {
            SaveUserDataFile();

            StringWriter sw = new StringWriter();
            XmlTextWriter writer = new XmlTextWriter(sw);
            writer.Formatting = Formatting.Indented;
			writer.WriteProcessingInstruction("xml", "version=\"1.0\" encoding=\"" + _game.TextEncoding.WebName + "\"");
            writer.WriteComment("DO NOT EDIT THIS FILE. It is automatically generated by the AGS Editor, changing it manually could break your game");
            writer.WriteStartElement(XML_ROOT_NODE_NAME);
            writer.WriteAttributeString(XML_ATTRIBUTE_VERSION, LATEST_XML_VERSION);
            writer.WriteAttributeString(XML_ATTRIBUTE_VERSION_INDEX, LATEST_XML_VERSION_INDEX.ToString());
            writer.WriteAttributeString(XML_ATTRIBUTE_EDITOR_VERSION, AGS.Types.Version.AGS_EDITOR_VERSION);

			_game.SavedXmlVersion = LATEST_XML_VERSION;
            _game.SavedXmlVersionIndex = LATEST_XML_VERSION_INDEX;
            _game.SavedXmlEncodingCodePage = _game.TextEncoding.CodePage;
            _game.ToXml(writer);

			Factory.Events.OnSavingGame(writer);

            writer.WriteEndElement();
            writer.Flush();

            string gameXml = sw.ToString();
            writer.Close();

            if (WriteMainGameFile(gameXml))
            {
                Factory.NativeProxy.SaveGame(_game);
            }
            else
            {
                return false;
            }

            DeleteObsoleteFilesFrom272();
			_game.FilesAddedOrRemoved = false;

            return true;
        }

        private bool WriteMainGameFile(string fileContents)
        {
            // First save the game into the temp file
            string tempFile = Path.GetTempFileName();
            using (StreamWriter fileOutput = new StreamWriter(tempFile, false, _game.TextEncoding))
            {
                try
                {
                    fileOutput.Write(fileContents);
                    fileOutput.Flush();
                }
                catch (Exception ex)
                {
                    Factory.GUIController.ShowMessage($"Unable to save new game data to '{tempFile}'. The error was: {ex.Message}", MessageBoxIcon.Warning);
                    return false;
                }
            }

            // Create a backup file, by moving previous game file into backup
            string gameFile = Utilities.ResolveSourcePath(GAME_FILE_NAME);
            string backupFile = $"{gameFile}.{BACKUP_EXTENSION}";
            try
            {
                if (File.Exists(gameFile))
                {
                    Utilities.TryDeleteFile(backupFile);
                    File.Move(gameFile, backupFile);
                }
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage($"Unable to create the backup file '{backupFile}'. The error was: {ex.Message}", MessageBoxIcon.Warning);
                return false;
            }

            // Finally try moving the saved temp game file into the project folder
            try
            {
                File.Move(tempFile, gameFile);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage($"Unable to create the game file '{gameFile}'. The error was: {ex.Message}", MessageBoxIcon.Warning);
                return false;
            }

            return true;
        }

        private void DeleteObsoleteFilesFrom272()
        {
            string[] filesToDelete = { "ac2game.ags", "ac2game.dta", "editor.dat", "acwin.dat", 
                                       "backup_ac2game.dta", "backup_editor.dat", "Edit this .AGSGame", 
                                       @"Compiled\music.vox" };

            foreach (string fileName in filesToDelete)
            {
                Utilities.TryDeleteFile(fileName);
            }
        }

        public void RunProcessAllGameTextsEvent(IGameTextProcessor processor, CompileMessages errors)
        {
            if (ProcessAllGameTexts != null)
            {
                ProcessAllGameTexts(processor, errors);
            }
        }

        private class CompileScriptsParameters
        {
            internal CompileMessages Errors { get; set; }
            internal bool RebuildAll { get; set; }

            internal CompileScriptsParameters(CompileMessages errors, bool rebuildAll)
            {
                this.Errors = errors;
                this.RebuildAll = rebuildAll;
            }
        }
    }
}
