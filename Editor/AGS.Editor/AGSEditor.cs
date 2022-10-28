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
using AGS.CScript.Compiler;
using AGS.Types;
using AGS.Types.Interfaces;
using AGS.Editor.Preferences;
using AGS.Editor.Utils;
using System.Net;

namespace AGS.Editor
{
    public class AGSEditor : ISourceControlIntegration
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
		public event GetSourceControlFileListHandler GetSourceControlFileList;

		public const string BUILT_IN_HEADER_FILE_NAME = "_BuiltInScriptHeader.ash";
        public const string OUTPUT_DIRECTORY = "Compiled";
        public const string DATA_OUTPUT_DIRECTORY = "Data"; // subfolder in OUTPUT_DIRECTORY for data file outputs
        public const string DEBUG_OUTPUT_DIRECTORY = "_Debug";
        //public const string DEBUG_EXE_FILE_NAME = "_debug.exe";
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
        */
        public const int    LATEST_XML_VERSION_INDEX = 3060020;
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
        private ISourceControlProvider _sourceControl = null;
        private AppSettings _applicationSettings = null;
        private Tasks _tasks = new Tasks();
        private IEngineCommunication _engineComms = new NamedPipesEngineCommunication();
        private DebugController _debugger;
		private bool _applicationStarted = false;
        private FileSystemWatcher _fileWatcher = null;
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

        public ISourceControlProvider SourceControlProvider
        {
            get { return _sourceControl; }

            set { _sourceControl = value; }
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
            _sourceControl = new SourceControlProvider();
            _debugger = new DebugController(_engineComms);
            _debugger.BreakAtLocation += new DebugController.BreakAtLocationHandler(_debugger_BreakAtLocation);

            _builtInScriptHeader = new Script(BUILT_IN_HEADER_FILE_NAME, Resources.ResourceManager.GetResourceAsString("agsdefns.sh"), true);
            AutoComplete.ConstructCache(_builtInScriptHeader, null);

            List<string> errors = new List<string>();
            Factory.NativeProxy.NewGameLoaded(Factory.AGSEditor.CurrentGame, errors);
            ReportGameLoad(errors);
        }

        public void ReportGameLoad(List<string> errors)
        {
            if (errors.Count == 1)
            {
                Factory.GUIController.ShowMessage(errors[0], MessageBoxIcon.Warning);
            }
            else if (errors.Count > 1)
            {
                Factory.GUIController.ShowOutputPanel(errors.ToArray(), "SpriteIcon");
                Factory.GUIController.ShowMessage("Game was loaded, but there were errors", MessageBoxIcon.Warning);
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
            try
            {
                ConfigurationManager.OpenExeConfiguration(ConfigurationUserLevel.PerUserRoamingAndLocal);
            }
            catch (ConfigurationErrorsException ex)
            {
                string filename = ex.Filename;
                Factory.GUIController.ShowMessage("Editor's configuration is corrupt and cannot be loaded. " +
                        "This could happen because of an improper program exit, a disk malfunction, or an invalid file edit. " +
                        "The configuration file will be deleted and user preferences reset to allow Editor start." +
                        "\n\n\nConfiguration file's location: " + filename,
                        MessageBoxIcon.Error);
                Utilities.TryDeleteFile(filename);
            }

            _applicationSettings = new AppSettings();
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

		public string[] GetFilesThatCanBePutUnderSourceControl()
        {
            List<string> files = new List<string>();

            files.Add(GAME_FILE_NAME);

            if (_game.Settings.BinaryFilesInSourceControl)
            {
                files.Add(SPRITE_FILE_NAME);
                files.Add(SPRITE_INDEX_FILE_NAME);
            }

            foreach (Script script in _game.RootScriptFolder.AllScriptsFlat)
            {
                files.Add(script.FileName);
            }

            foreach (UnloadedRoom room in _game.RootRoomFolder.AllItemsFlat)
            {
                files.Add(room.FileName);
                files.Add(room.ScriptFileName);
            }

            foreach (Translation trans in _game.Translations)
            {
                files.Add(trans.FileName);
            }

			foreach (AGS.Types.Font font in _game.Fonts)
			{
				if (File.Exists(font.TTFFileName))
				{
					files.Add(font.TTFFileName);
				}
				else if (File.Exists(font.WFNFileName))
				{
					files.Add(font.WFNFileName);
				}
			}

            for (int i = 0; i < files.Count; i++)
            {
                files[i] = Path.GetFullPath(files[i]);
            }

			if (GetSourceControlFileList != null)
			{
				GetSourceControlFileList(files);
			}

            // Remove any VSSVER.SCC files that have crept in
            for (int i = 0; i < files.Count; i++)
            {
                if (files[i].ToLower().EndsWith(".scc"))
                {
                    files.RemoveAt(i);
                }
            }

            return files.ToArray();
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

		public void RenameFileOnDiskAndSourceControl(string currentName, string newName)
		{
			_sourceControl.RenameFileOnDiskAndInSourceControl(currentName, newName);
		}

		public void RegisterFileIconAssociation(string fileExtension, string iconKey)
		{
			CheckinsDialog.RegisterFileIcon(fileExtension, iconKey);
		}

		public void DeleteFileOnDiskAndSourceControl(string fileName)
		{
			DeleteFileOnDiskAndSourceControl(new string[] { fileName });
		}

		public void DeleteFileOnDiskAndSourceControl(string[] fileNames)
		{
			string[] fullPathNames = new string[fileNames.Length];
			for (int i = 0; i < fileNames.Length; i++)
			{
				fullPathNames[i] = Path.GetFullPath(fileNames[i]);
			}

			List<string> filesUnderControl = new List<string>();
			SourceControlFileStatus[] statuses = _sourceControl.GetFileStatuses(fullPathNames);
			for (int i = 0; i < fullPathNames.Length; i++)
			{
				if ((statuses[i] & SourceControlFileStatus.Controlled) != 0)
				{
					filesUnderControl.Add(fullPathNames[i]);
				}
			}

			if (filesUnderControl.Count > 0)
			{
				if (Factory.GUIController.ShowQuestion("Do you also want to delete the file(s) from the Source Control repository?") == DialogResult.Yes)
				{
					try
					{
						_sourceControl.RemoveFiles(filesUnderControl.ToArray());
					}
					catch (SourceControlException ex)
					{
						Factory.GUIController.ShowMessage("Unable to delete files from source control: " + ex.SccErrorMessage, MessageBoxIcon.Warning);
					}
				}
			}

			foreach (string fileName in fullPathNames)
			{
                Utilities.TryDeleteFile(fileName);
			}
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

			try
			{
				if (fileNamesWithFullPaths.Count > 0)
				{
					SourceControlFileStatus[] statuses = Factory.AGSEditor.SourceControlProvider.GetFileStatuses(fileNamesWithFullPaths.ToArray());
					List<string> filesToCheckOut = new List<string>();
					for (int i = 0; i < statuses.Length; i++)
					{
						if (((statuses[i] & SourceControlFileStatus.Controlled) != 0) &&
                            ((statuses[i] & SourceControlFileStatus.Deleted) == 0) &&
							((statuses[i] & SourceControlFileStatus.CheckedOutByMe) == 0))
						{
							filesToCheckOut.Add(fileNamesWithFullPaths[i]);
						}
					}

					if (filesToCheckOut.Count > 0)
					{
						if (!Factory.GUIController.ShowCheckOutDialog(filesToCheckOut))
						{
							return false;
						}
					}
				}
			}
			catch (SourceControlException ex)
			{
				Factory.GUIController.ShowMessage("Unable to check source control status: " + ex.SccErrorMessage, MessageBoxIcon.Warning);
				return false;
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
            if (_fileWatcher != null)
            {
                _fileWatcher.EnableRaisingEvents = false;
                _fileWatcher.Dispose();
                _fileWatcher = null;
            }

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

        public void RefreshEditorAfterGameLoad(Game newGame, List<string> errors)
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
            
            if (System.Environment.OSVersion.Platform == PlatformID.Win32NT)
            {
                // file system watcher only works on NT
                _fileWatcher = new FileSystemWatcher(newGame.DirectoryPath);
                _fileWatcher.Changed += new FileSystemEventHandler(_fileWatcher_Changed);
                _fileWatcher.NotifyFilter = NotifyFilters.LastWrite;
                _fileWatcher.EnableRaisingEvents = true;
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

        private void _fileWatcher_Changed(object sender, FileSystemEventArgs e)
        {
            Factory.Events.OnFileChangedInGameFolder(e.Name);
        }

		private void DefineMacrosAccordingToGameSettings(IPreprocessor preprocessor)
		{
			preprocessor.DefineMacro("AGS_NEW_STRINGS", "1");
			preprocessor.DefineMacro("AGS_SUPPORTS_IFVER", "1");
			if (_game.Settings.DebugMode)
			{
				preprocessor.DefineMacro("DEBUG", "1");
			}
			if (_game.Settings.EnforceObjectBasedScript)
			{
				preprocessor.DefineMacro("STRICT", "1");
			}
			if (_game.Settings.LeftToRightPrecedence)
			{
				preprocessor.DefineMacro("LRPRECEDENCE", "1");
			}
			if (_game.Settings.EnforceNewStrings)
			{
				preprocessor.DefineMacro("STRICT_STRINGS", "1");
			}
            if (_game.Settings.EnforceNewAudio)
            {
                preprocessor.DefineMacro("STRICT_AUDIO", "1");
            }
            if (!_game.Settings.UseOldCustomDialogOptionsAPI)
            {
                preprocessor.DefineMacro("NEW_DIALOGOPTS_API", "1");
            }
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

		/// <summary>
		/// Preprocesses and then compiles the script using the supplied headers.
		/// </summary>
		public void CompileScript(Script script, List<Script> headers, CompileMessages errors)
		{
			IPreprocessor preprocessor = CompilerFactory.CreatePreprocessor(AGS.Types.Version.AGS_EDITOR_VERSION);
			DefineMacrosAccordingToGameSettings(preprocessor);

			List<string> preProcessedCode = new List<string>();
			foreach (Script header in headers)
			{
				preProcessedCode.Add(preprocessor.Preprocess(header.Text, header.FileName));
			}

			preProcessedCode.Add(preprocessor.Preprocess(script.Text, script.FileName));

#if DEBUG
			// TODO: REMOVE BEFORE DISTRIBUTION
/*			if (true)
			{
                string wholeScript = string.Join("\n", preProcessedCode.ToArray());
				IScriptCompiler compiler = CompilerFactory.CreateScriptCompiler();
				CompileResults output = compiler.CompileScript(wholeScript);
				preprocessor.Results.AddRange(output);
			}*/
#endif

			if (preprocessor.Results.Count > 0)
			{
				foreach (AGS.CScript.Compiler.Error error in preprocessor.Results)
				{
					CompileError newError = new CompileError(error.Message, error.ScriptName, error.LineNumber);
					if (errors == null)
					{
						throw newError;
					}
					errors.Add(newError);
				}
			}
			else
			{
				Factory.NativeProxy.CompileScript(script, preProcessedCode.ToArray(), _game);
			}
		}

        private Script CompileDialogs(CompileMessages errors, bool rebuildAll)
        {
            DialogScriptConverter dialogConverter = new DialogScriptConverter();
            string dialogScriptsText = dialogConverter.ConvertGameDialogScripts(_game, errors, rebuildAll);
            Script dialogScripts = new Script(Script.DIALOG_SCRIPTS_FILE_NAME, dialogScriptsText, false);
            Script globalScript = _game.RootScriptFolder.GetScriptByFileName(Script.GLOBAL_SCRIPT_FILE_NAME, true);
            if (!System.Text.RegularExpressions.Regex.IsMatch(globalScript.Text, @"function\s+dialog_request\s*\("))
            {
                // A dialog_request must exist in the global script, otherwise
                // the dialogs script fails to load at run-time
                globalScript.Text += Environment.NewLine + "function dialog_request(int param) {" + Environment.NewLine + "}";
            }
            return dialogScripts;
        }

        private object CompileScripts(IWorkProgress progress, object parameter)
        {
            CompileScriptsParameters parameters = (CompileScriptsParameters)parameter;
            CompileMessages errors = parameters.Errors;
            CompileMessage errorToReturn = null;
            RegenerateScriptHeader(null);
            List<Script> headers = GetInternalScriptHeaders();

            try
            {
                Script dialogScripts = CompileDialogs(errors, parameters.RebuildAll);

                _game.ScriptsToCompile = new ScriptsAndHeaders();

                foreach (Script script in GetInternalScriptModules())
                {
                    CompileScript(script, headers, errors);
                    _game.ScriptsToCompile.Add(new ScriptAndHeader(null, script));
                }

                foreach (ScriptAndHeader scripts in _game.RootScriptFolder.AllItemsFlat)
                {
                    headers.Add(scripts.Header);
                    CompileScript(scripts.Script, headers, errors);
                    _game.ScriptsToCompile.Add(scripts);					
                }

                CompileScript(dialogScripts, headers, errors);
                _game.ScriptsToCompile.Add(new ScriptAndHeader(null, dialogScripts));
			}
            catch (CompileMessage ex)
            {
                errorToReturn = ex;
            }

            if (ExtraCompilationStep != null)
            {
                ExtraCompilationStep(errors);
            }

            return errorToReturn;
        }

        private void DeleteAnyExistingSplitResourceFiles()
        {
            string dir = Path.Combine(OUTPUT_DIRECTORY, DATA_OUTPUT_DIRECTORY);
            foreach (string fileName in Utilities.GetDirectoryFileList(dir, this.BaseGameFileName + ".0*"))
            {
                Utilities.TryDeleteFile(fileName);
            }
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
            CompileMessages errors = new CompileMessages();

            Utilities.EnsureStandardSubFoldersExist();

            forceRebuild |= _game.WorkspaceState.RequiresRebuild;

            if (PreCompileGame != null)
            {
				PreCompileGameEventArgs evArgs = new PreCompileGameEventArgs(forceRebuild);
				evArgs.Errors = errors;

                PreCompileGame(evArgs);

                if (!evArgs.AllowCompilation)
                {
                    Factory.GUIController.ShowOutputPanel(errors);
					ReportErrorsIfAppropriate(errors);
                    return errors;
                }
            }

            RunPreCompilationChecks(errors);

			if (!errors.HasErrors)
			{
				CompileMessage result = (CompileMessage)BusyDialog.Show("Please wait while your scripts are compiled...", new BusyDialog.ProcessingHandler(CompileScripts), new CompileScriptsParameters(errors, forceRebuild));
				if (result != null)
				{
					errors.Add(result);
				}
				else if (!errors.HasErrors)
				{
					if (createMiniExeForDebug)
					{
						CreateMiniEXEForDebugging(errors);
					}
					else
					{
						CreateCompiledFiles(errors, forceRebuild);
					}
                    _game.WorkspaceState.RequiresRebuild = false;
                }
			}

            Factory.GUIController.ShowOutputPanel(errors);

			ReportErrorsIfAppropriate(errors);

            return errors;
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

        public void SetMODMusicFlag()
        {
            foreach (AudioClip clip in _game.RootAudioClipFolder.GetAllAudioClipsFromAllSubFolders())
            {
                if (clip.FileType == AudioClipFileType.MOD)
                {
                    _game.Settings.HasMODMusic = true;
                    return;
                }
            }
            _game.Settings.HasMODMusic = false;
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

        private void WriteCustomPathToConfig(string section, string key, string cfg_path, bool use_custom_path, string custom_path)
        {
            string path_value = ""; // no value
            if (use_custom_path)
            {
                if (String.IsNullOrEmpty(custom_path))
                    path_value = "."; // same directory
                else
                    path_value = custom_path;
            }
            NativeProxy.WritePrivateProfileString(section, key, path_value, cfg_path);
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
        /// Writes up-to-date game information into configuration file.
        /// This updates only values that strongly depend on game properties,
        /// and does not affect user settings.
        /// </summary>
		public void WriteConfigFile(string outputDir, bool resetFile = true)
		{
            string configFilePath = Path.Combine(outputDir, CONFIG_FILE_NAME);
            if (resetFile)
                Utilities.TryDeleteFile(configFilePath);

            if (_game.Settings.LetterboxMode)
            {
                NativeProxy.WritePrivateProfileString("misc", "defaultres", ((int)_game.Settings.LegacyLetterboxResolution).ToString(), configFilePath);
                NativeProxy.WritePrivateProfileString("misc", "letterbox", "1", configFilePath);
                NativeProxy.WritePrivateProfileString("misc", "game_width", null, configFilePath);
                NativeProxy.WritePrivateProfileString("misc", "game_height", null, configFilePath);
            }
            else
            {
                NativeProxy.WritePrivateProfileString("misc", "defaultres", null, configFilePath);
                NativeProxy.WritePrivateProfileString("misc", "letterbox", null, configFilePath);
                NativeProxy.WritePrivateProfileString("misc", "game_width", _game.Settings.CustomResolution.Width.ToString(), configFilePath);
                NativeProxy.WritePrivateProfileString("misc", "game_height", _game.Settings.CustomResolution.Height.ToString(), configFilePath);
            }
			NativeProxy.WritePrivateProfileString("misc", "gamecolordepth", (((int)_game.Settings.ColorDepth) * 8).ToString(), configFilePath);

            NativeProxy.WritePrivateProfileString("graphics", "driver", GetGfxDriverConfigID(_game.DefaultSetup.GraphicsDriver), configFilePath);
            NativeProxy.WritePrivateProfileString("graphics", "windowed", _game.DefaultSetup.Windowed ? "1" : "0", configFilePath);
            NativeProxy.WritePrivateProfileString("graphics", "fullscreen",
                _game.DefaultSetup.FullscreenDesktop ? "full_window" : "desktop", configFilePath);
            if (_game.DefaultSetup.GameScaling == GameScaling.Integer)
                NativeProxy.WritePrivateProfileString("graphics", "window",
                    String.Format("x{0}", _game.DefaultSetup.GameScalingMultiplier), configFilePath);
            else
                NativeProxy.WritePrivateProfileString("graphics", "window", "desktop", configFilePath);

            NativeProxy.WritePrivateProfileString("graphics", "game_scale_fs", MakeGameScalingConfig(_game.DefaultSetup.FullscreenGameScaling), configFilePath);
            NativeProxy.WritePrivateProfileString("graphics", "game_scale_win", MakeGameScalingConfig(_game.DefaultSetup.GameScaling), configFilePath);

            NativeProxy.WritePrivateProfileString("graphics", "filter", _game.DefaultSetup.GraphicsFilter, configFilePath);
            NativeProxy.WritePrivateProfileString("graphics", "vsync", _game.DefaultSetup.VSync ? "1" : "0", configFilePath);
            NativeProxy.WritePrivateProfileString("misc", "antialias", _game.DefaultSetup.AAScaledSprites ? "1" : "0", configFilePath);
            bool render_at_screenres = _game.Settings.RenderAtScreenResolution == RenderAtScreenResolution.UserDefined ?
                _game.DefaultSetup.RenderAtScreenResolution : _game.Settings.RenderAtScreenResolution == RenderAtScreenResolution.True;
            NativeProxy.WritePrivateProfileString("graphics", "render_at_screenres", render_at_screenres ? "1" : "0", configFilePath);
            int rotation = (int)_game.DefaultSetup.Rotation;
            NativeProxy.WritePrivateProfileString("graphics", "rotation", rotation.ToString(), configFilePath);

            bool audio_enabled = _game.DefaultSetup.DigitalSound != RuntimeAudioDriver.Disabled;
            NativeProxy.WritePrivateProfileString("sound", "enabled", audio_enabled ? "1" : "0", configFilePath);
            NativeProxy.WritePrivateProfileString("sound", "driver", "", configFilePath); // always default
            NativeProxy.WritePrivateProfileString("sound", "usespeech", _game.DefaultSetup.UseVoicePack ? "1" : "0", configFilePath);

            int control_enabled = (int)_game.DefaultSetup.ControlEnabled;
            NativeProxy.WritePrivateProfileString("language", "translation", _game.DefaultSetup.Translation, configFilePath);
            NativeProxy.WritePrivateProfileString("mouse", "auto_lock", _game.DefaultSetup.AutoLockMouse ? "1" : "0", configFilePath);
            NativeProxy.WritePrivateProfileString("mouse", "speed", _game.DefaultSetup.MouseSpeed.ToString(CultureInfo.InvariantCulture), configFilePath);
            NativeProxy.WritePrivateProfileString("mouse", "control_enabled", control_enabled.ToString(), configFilePath);

            int emulate_mouse = (int) _game.DefaultSetup.EmulateMouse;
            NativeProxy.WritePrivateProfileString("touch", "emulate_mouse", emulate_mouse.ToString(), configFilePath);

            // Note: sprite cache size is written in KB (while we have it in MB on the editor pane)
            NativeProxy.WritePrivateProfileString("misc", "cachemax", (_game.DefaultSetup.SpriteCacheSize * 1024).ToString(), configFilePath);
            WriteCustomPathToConfig("misc", "user_data_dir", configFilePath, _game.DefaultSetup.UseCustomSavePath, _game.DefaultSetup.CustomSavePath);
            WriteCustomPathToConfig("misc", "shared_data_dir", configFilePath, _game.DefaultSetup.UseCustomAppDataPath, _game.DefaultSetup.CustomAppDataPath);
            NativeProxy.WritePrivateProfileString("misc", "titletext", _game.DefaultSetup.TitleText, configFilePath);

            if(_game.Settings.DebugMode) // Do not write show_fps in a release build, this is only intended for the developer
            {
                NativeProxy.WritePrivateProfileString("misc", "show_fps", _game.DefaultSetup.ShowFPS ? "1" : "0", configFilePath);
            }
        }

        private void SaveUserDataFile()
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
			WriteConfigFile(Path.Combine(OUTPUT_DIRECTORY, DATA_OUTPUT_DIRECTORY));

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

            string gameFile = Utilities.ResolveSourcePath(GAME_FILE_NAME);
            string backupFile = $"{gameFile}.{BACKUP_EXTENSION}";

            try
            {
                Utilities.TryDeleteFile(backupFile);
                File.Move(gameFile, backupFile);
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage($"Unable to create the backup file '{backupFile}'. The error was: {ex.Message}", MessageBoxIcon.Warning);
                return false;
            }

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