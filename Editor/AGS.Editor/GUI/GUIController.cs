using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using Microsoft.Win32;
using AGS.Editor.Components;
using AGS.Types;
using AGS.Types.Enums;
using AGS.Types.AutoComplete;
using AGS.Types.Interfaces;
using AGS.Editor.Preferences;
using System.Drawing.Text;
using System.Linq;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public class GUIController : IGUIController
    {
        public const string FILE_MENU_ID = "fileToolStripMenuItem";
        public const string HELP_MENU_ID = "HelpMenu";
        private const string CONTROL_ID_SPLIT = "^!^";
		private const string ROOM_TEMPLATE_ID_FILE = "rtemplate.dat";
		private const int ROOM_TEMPLATE_ID_FILE_SIGNATURE = 0x74673812;
        private const string WINDOW_CONFIG_FILENAME = "WindowConfig.json";

        public delegate void PropertyObjectChangedHandler(object newPropertyObject);
        public event PropertyObjectChangedHandler OnPropertyObjectChanged;
        public delegate bool QueryEditorShutdownHandler();
        public event QueryEditorShutdownHandler QueryEditorShutdown;
        public delegate void EditorShutdownHandler();
        public event EditorShutdownHandler OnEditorShutdown;
        public delegate void ZoomToFileHandler(ZoomToFileEventArgs evArgs);
        public event ZoomToFileHandler OnZoomToFile;
        public delegate void GetScriptHandler(string fileName, ref Script script);
        public event GetScriptHandler OnGetScript;
        public delegate void AttemptToEditScriptHandler(ref bool allowEdit);
        public static event AttemptToEditScriptHandler AttemptToEditScript;
        public delegate void GetScriptEditorControlHandler(GetScriptEditorControlEventArgs evArgs);
        public event GetScriptEditorControlHandler OnGetScriptEditorControl;
        public delegate void ScriptChangedHandler(Script script);
        public event ScriptChangedHandler OnScriptChanged;
        public delegate void LaunchHelpHandler(string keyword);
        public event LaunchHelpHandler OnLaunchHelp;
        public event EventHandler OnMainWindowActivated;

        private delegate void UpdateStatusBarTextDelegate(string text);
        private delegate void ZoomToFileDelegate(string fileName, ZoomToFileZoomType zoomType, int lineNumber, bool isDebugExecutionPoint, bool selectWholeLine, string errorMessage, bool activateEditor);
        private delegate void ShowCallStackDelegate(DebugCallStack callStack);
        private delegate void ShowFindSymbolResultsDelegate(List<ScriptTokenReference> results);
        private delegate void NotifyWatchVariablesDelegate();
        private delegate void NotifySetAutoLocalVariables(DebugCallStack callStack);

        private frmMain _mainForm;
        private LogPanel _pnlEngineLog;
        private Dictionary<string, IEditorComponent> _menuItems;
        private ImageList _imageList = new ImageList();
        private ProjectTree _treeManager;
        private ToolBarManager _toolBarManager;
        private MainMenuManager _menuManager;
        private WindowsMenuManager _windowsMenuManager;
        private string _titleBarPrefix;
        private AGSEditor _agsEditor;
        private InteractiveTasks _interactiveTasks;
        private bool _exitFromWelcomeScreen = false;
		private bool _batchProcessShutdown = false;
		private int _lastSelectedSprite = 0;
		private string _lastImportDirectory = null;
        CommandLineOptions _commandOptions = new CommandLineOptions();
        private bool _messageLoopStarted = false;
        private int _systemDpi = 0;

        // Custom color table for the ColorDialog
        private int[] _customColors = null;

        private GUIController()
        {
            InstalledFonts = new InstalledFontCollection().Families.ToDictionary(t => t.Name, t => t.Name);
            _menuItems = new Dictionary<string, IEditorComponent>();
            Factory.Events.GamePostLoad += Events_GamePostLoad;
        }

        private static GUIController _instance;

        public static GUIController Instance
        {
            get
            {
                if (_instance == null)
                {
                    _instance = new GUIController();
                }
                return _instance;
            }
        }

        public IntPtr TopLevelWindowHandle
        {
            get { return _mainForm.Handle; }
        }

		public Icon StandardEditorIcon
		{
			get { return _mainForm.Icon; }
		}

        public ProjectTree ProjectTree
        {
            get { return _treeManager; }
        }

		IProjectTree IGUIController.ProjectTree
		{
			get { return _treeManager; }
		}

		public InteractiveTasks InteractiveTasks
        {
            get { return _interactiveTasks; }
        }

        public ToolBarManager ToolBarManager
        {
            get { return _toolBarManager; }
        }

        public MainMenuManager MenuManager
        {
            get { return _menuManager; }
        }

        public ImageList ImageList
        {
            get { return _imageList; }
        }

		public string FileMenuID
		{
			get { return FILE_MENU_ID; }
		}

        public Icon MainIcon
        {
            get { return _mainForm.Icon; }
        }

        public IColorThemes ColorThemes { get; private set; }
        public IObjectConfig WindowConfig { get; private set; }

        public Dictionary<string, string> InstalledFonts { get; private set; }

        public void ShowMessage(string message, MessageBoxIconType icon)
		{
			MessageBoxIcon windowsFormsIcon = MessageBoxIcon.Information;
			if (icon == MessageBoxIconType.Warning)
			{
				windowsFormsIcon = MessageBoxIcon.Warning;
			}
			else if (icon == MessageBoxIconType.Error)
			{
				windowsFormsIcon = MessageBoxIcon.Error;
			}
			ShowMessage(message, windowsFormsIcon);
		}

        public void ShowMessage(string message, MessageBoxIcon icon)
        {
            if (StdConsoleWriter.IsEnabled)
            {
                StdConsoleWriter.WriteLine(message);
                return;
            }

            if ((Form.ActiveForm == null) || (Form.ActiveForm.InvokeRequired))
            {
                MessageBox.Show(message, "Adventure Game Studio", MessageBoxButtons.OK, icon);
            }
            else
            {
                MessageBox.Show(Form.ActiveForm, message, "Adventure Game Studio", MessageBoxButtons.OK, icon);
            }
        }

        public DialogResult ShowQuestion(string message)
        {
            return ShowQuestion(message, MessageBoxIcon.Question);
        }

        public DialogResult ShowQuestion(string message, MessageBoxIcon icon)
        {
            return MessageBox.Show(message, "Adventure Game Studio", MessageBoxButtons.YesNo, icon);
        }

        public int GetSystemDPI()
        {
            if (_systemDpi == 0)
            {
                Graphics g = _mainForm.CreateGraphics();
                _systemDpi = (int)g.DpiX;
                g.Dispose();
            }
            return _systemDpi;
        }

        public int AdjustSizeFrom96DpiToSystemDpi(int sizeAt96Dpi)
        {
            return (sizeAt96Dpi * GetSystemDPI()) / 96;
        }

        public void CustomPropertiesEditor(CustomProperties properties, CustomPropertyAppliesTo showPropertiesOfType)
        {
            AGS.Editor.CustomPropertiesEditor editor = new CustomPropertiesEditor(_agsEditor.CurrentGame.PropertySchema, properties, showPropertiesOfType);
            editor.ShowDialog();
            editor.Dispose();
        }

        public void Invoke(Delegate method, params object[] args)
        {
            _mainForm.Invoke(method, args);
        }

        public bool InvokeRequired
        {
            get { return _mainForm.InvokeRequired; }
        }

        public void GameNameUpdated()
        {
            _mainForm.Text = _titleBarPrefix + _agsEditor.CurrentGame.Settings.GameName + " - AGS Editor";
        }

        public void SetTitleBarPrefix(string prefix)
        {
            _titleBarPrefix = prefix;
            GameNameUpdated();
        }

        public void SetMenuItemEnabled(IEditorComponent plugin, string id, bool enabled)
        {
            string commandID = GetMenuCommandID(id, plugin);
            _menuManager.SetMenuItemEnabled(commandID, enabled);
        }

        public void ReplaceMenuSubCommands(IEditorComponent plugin, string oldCommandId, IList<MenuCommand> commands)
        {
            string commandID = GetMenuCommandID(oldCommandId, plugin);
            MenuCommand menuCommand = _menuManager.GetCommandById(commandID);
            UnregisterMenuItems(menuCommand.SubCommands);
            RegisterMenuItems(plugin, commands); // fixes command ID prefixes
            menuCommand.SubCommands = commands;
            _menuManager.ReplaceMenuItemSubcommands(commandID, commands);
        }

        public void AddMenu(IEditorComponent plugin, string id, string title)
        {
            _menuManager.AddMenu(id, title);
        }

		public void AddMenu(IEditorComponent plugin, string id, string title, string insertAfterMenu)
		{
			_menuManager.AddMenu(id, title, insertAfterMenu);
		}

        private void RegisterMenuItems(IEditorComponent plugin, IList<MenuCommand> commands)
        {
            foreach (MenuCommand command in commands)
            {
                if (command.ID != null)
                {
                    RegisterMenuCommand(command.ID, plugin);
                }

                command.IDPrefix = plugin.ComponentID + CONTROL_ID_SPLIT;

                if (command.SubCommands != null && command.SubCommands.Count > 0)
                {
                    RegisterMenuItems(plugin, command.SubCommands);
                }
            }
        }

        private void UnregisterMenuItems(IList<MenuCommand> commands)
        {
            foreach (MenuCommand command in commands)
            {
                if (command.ID != null)
                {
                    UnregisterMenuCommand(command.ID);
                }

                if (command.SubCommands != null && command.SubCommands.Count > 0)
                {
                    UnregisterMenuItems(command.SubCommands);
                }
            }
        }


        public void AddMenuItems(IEditorComponent plugin, MenuCommands commands)
        {
            if (commands.Commands.Count > 0)
            {
                RegisterMenuItems(plugin, commands.Commands);
                _menuManager.AddMenuCommandGroup(commands);
            }
        }

		public IScriptEditorControl CreateScriptEditor(Point position, Size size)
		{
			if (!_messageLoopStarted)
			{
				// This is because the splash screen uses a different message loop,
				// the scintilla control gets reset when the new message loop starts
				// if it has already been created.
				throw new AGSEditorException("A script editor cannot be created from within a component constructor because the GUI system is not yet initialized. You should do a just-in-time creation of your ContentPane rather than creating it at component construction.");
			}
			ScintillaWrapper scintilla = new ScintillaWrapper();
			scintilla.Location = position;
			scintilla.Size = size;
			scintilla.SetKeyWords(Constants.SCRIPT_KEY_WORDS);
			scintilla.SetFillupKeys(Constants.AUTOCOMPLETE_ACCEPT_KEYS);
			scintilla.AutoCompleteEnabled = true;
			scintilla.AutoSpaceAfterComma = true;
			scintilla.CallTipsEnabled = true;
			return scintilla;
		}

		public MenuCommand CreateMenuCommand(IEditorComponent component, string commandID, string commandName)
		{
			MenuCommand newCommand = new MenuCommand(commandID, commandName);
			newCommand.ID = RegisterMenuCommand(commandID, component);
			return newCommand;
		}

        public void RemoveMenuItems(MenuCommands commands)
        {
			_menuManager.RemoveMenuCommandGroup(commands);
        }

        public void RegisterIcon(string key, Icon icon)
        {
            _imageList.Images.Add(key, icon);
        }

        public void RegisterImage(string key, Image image)
        {
            _imageList.Images.Add(key, image);
        }

        public void ResetWindowPanes()
        {
            _mainForm.ResetLayoutToDefaults();
        }

        /// <summary>
        /// Adds a persistent dock pane to the application window.
        /// </summary>
        public void AddDockPane(DockContent pane, DockData defaultDock)
        {
            // Update window menu and layout managers
            _mainForm.AddDockPane(pane, defaultDock);
            _windowsMenuManager.AddPersistentItems(_mainForm.DockPanes);
        }

        /// <summary>
        /// Adds or shows existing ContentDocument on the available dock site.
        /// </summary>
        public void AddOrShowPane(ContentDocument pane)
        {
            _mainForm.AddOrShowPane(pane);
        }

        /// <summary>
        /// Removes existing ContentDocument.
        /// </summary>
        public void RemovePaneIfExists(ContentDocument pane)
        {
            _mainForm.RemovePaneIfExists(pane);
        }

        /// <summary>
        /// Gets the list of the persistent dock panes.
        /// </summary>
        public IList<DockContent> DockPanes
        {
            get
            {
                return _mainForm.DockPanes;
            }
        }

        /// <summary>
        /// Gets the list of existing ContentDocuments.
        /// </summary>
        public IList<ContentDocument> Panes
        {
            get
            {
                return _mainForm.Panes;
            }
        }

        public void ShowOutputPanel(CompileMessages errors)
        {
            _mainForm.pnlOutput.ErrorsToList = errors;
            if (errors.Count > 0)
            {
                if (StdConsoleWriter.IsEnabled)
                {
                    foreach (CompileError cerr in errors.Errors)
                    {
                        StdConsoleWriter.WriteLine(cerr.AsString);
                    }
                }
                _mainForm.pnlOutput.Show();
            }
        }

        public void ShowOutputPanel(string[] messages, string imageKey = "BuildIcon")
        {
            if (StdConsoleWriter.IsEnabled)
            {
                foreach(string msg in messages)
                {
                    StdConsoleWriter.WriteLine(msg);
                }
            }
            _mainForm.pnlOutput.SetMessages(messages, imageKey);
            _mainForm.pnlOutput.Show();
        }

        public void ShowOutputPanel(string message, string imageKey = "BuildIcon")
        {
            StdConsoleWriter.WriteLine(message);
            _mainForm.pnlOutput.SetMessage(message, imageKey);
            _mainForm.pnlOutput.Show();
        }

        public void ClearOutputPanel()
        {
            _mainForm.pnlOutput.ErrorsToList = null;
        }

        public void HideOutputPanel()
        {
            if (_mainForm.pnlOutput.InvokeRequired)
            {
                _mainForm.pnlOutput.Invoke(new Action(HideOutputPanel));
                return;
            }

            _mainForm.pnlOutput.Hide();
        }

        public void ShowCallStack(DebugCallStack callStack)
        {
            if (_mainForm.pnlCallStack.InvokeRequired)
            {
                _mainForm.pnlCallStack.Invoke(new ShowCallStackDelegate(ShowCallStack), callStack);
                return;
            }

            _mainForm.pnlCallStack.CallStack = callStack;
            _mainForm.pnlCallStack.Show();
        }

        public void ClearCallStack()
        {
            _mainForm.pnlCallStack.CallStack = null;
        }

        public void HideCallStack()
        {
            _mainForm.pnlCallStack.Hide();
        }

        public void ShowFindSymbolResults(List<ScriptTokenReference> results)
        {            
            if (_mainForm.pnlFindResults.InvokeRequired)
            {
                _mainForm.pnlFindResults.Invoke(new ShowFindSymbolResultsDelegate(ShowFindSymbolResults), results);
                return;
            }

            _mainForm.pnlFindResults.Results = results;
            _mainForm.pnlFindResults.Show();
        }

        public void ClearFindSymbolResults()
        {
            _mainForm.pnlFindResults.Results = null;
        }

        public void HideFindSymbolResults()
        {
            _mainForm.pnlFindResults.Hide();
        }

        public void AddVariableToWatchPanel(string var_name)
        {
            _mainForm.pnlWatchVariables.AddVariableToWatchList(var_name);
            if (_mainForm.pnlWatchVariables.IsHidden)
                return;
            _mainForm.pnlWatchVariables.Show();
        }

        public void ShowWatchVariablesPanel(bool ifEnabled)
        {
            if (ifEnabled && _mainForm.pnlWatchVariables.IsHidden)
                return;
            _mainForm.pnlWatchVariables.Show();
        }

        public void SetLogPanel(LogPanel pnlEngineLog)
        {
            _pnlEngineLog = pnlEngineLog;
        }

        public void ShowLogPanel(bool ifEnabled)
        {
            if (_pnlEngineLog == null || (ifEnabled && _pnlEngineLog.IsHidden))
                return;
            _pnlEngineLog.Show();
        }

        public void ClearEngineLogMessages()
        {
            _pnlEngineLog?.Clear();
        }

        public void PrintEngineLog(string message, LogGroup group, LogLevel level)
        {
            _pnlEngineLog?.WriteLogMessage(message, group, level);
        }

        public void SetAutoLocalVariables(DebugCallStack callStack)
        {
            if (_mainForm.pnlWatchVariables.InvokeRequired)
            {
                _mainForm.pnlWatchVariables.Invoke(new NotifySetAutoLocalVariables(SetAutoLocalVariables), callStack);
                return;
            }

            _mainForm.pnlWatchVariables.SetAutoLocalVariables(callStack);
        }

        public void NotifyWatchVariables()
        {
            if (_mainForm.pnlWatchVariables.InvokeRequired)
            {
                _mainForm.pnlWatchVariables.Invoke(new NotifyWatchVariablesDelegate(NotifyWatchVariables));
                return;
            }

            _mainForm.pnlWatchVariables.UpdateAllWatches();
        }

        public ContentDocument ActivePane
        {
            get { return _mainForm.ActivePane; }
        }

        public void LaunchHelpForKeyword(string keyword)
        {
            if (OnLaunchHelp != null)
            {
                OnLaunchHelp(keyword);
            }
        }

		void IGUIController.ShowHelpFile(string keyword)
		{
			LaunchHelpForKeyword(keyword);
		}

		void IGUIController.OpenEditorForScript(string fileName, int lineNumber)
		{
			ZoomToFile(fileName, lineNumber);
		}

        /// <summary>
        /// Tries to open a pane that contains an object described by the given script symbol.
        /// * If a symbol is user-defined, then opens a corresponding user script.
        /// * If a symbol is a game item, then opens that item's edit pane.
        /// * If a symbol is a part of a scripting API, then launches the manual.
        /// </summary>
        public void ZoomToAnyScriptSymbol(string scriptName, string symbolName, string typeName, int scriptCharPos)
        {
            if (scriptName == AGSEditor.BUILT_IN_HEADER_FILE_NAME)
            {
                if (symbolName == "player")
                {
                    CharactersComponent charactersComponent = Factory.ComponentController.FindComponent<CharactersComponent>();
                    charactersComponent.ShowPlayerCharacter();
                }
                else
                {
                    Factory.GUIController.LaunchHelpForKeyword(symbolName);
                }
            }
            else if (scriptName == Tasks.AUTO_GENERATED_HEADER_NAME)
            {
                if (!string.IsNullOrEmpty(typeName))
                {
                    ZoomToComponentObject(typeName, symbolName);
                }
                else
                {
                    Factory.GUIController.ShowMessage("This symbol is internally defined by AGS.", MessageBoxIcon.Information);
                }
            }
            else if (scriptName == GlobalVariablesComponent.GLOBAL_VARS_HEADER_FILE_NAME)
            {
                IGlobalVariablesController globalVariables = (IGlobalVariablesController)Factory.ComponentController.FindComponentThatImplementsInterface(typeof(IGlobalVariablesController));
                globalVariables.SelectGlobalVariable(symbolName);
            }
            else
            {
                Factory.GUIController.ZoomToFile(scriptName, ZoomToFileZoomType.ZoomToCharacterPosition, scriptCharPos);
            }
        }

        public void ZoomToComponentObject(string typeName, string objectName, bool selectEventsTab = false)
        {
            BaseComponent component = Factory.ComponentController.FindComponentThatManageScriptElement(typeName) as BaseComponent;
            if (component != null)
            {
                if (component.ShowItemPaneByName(objectName))
                {
                    if (selectEventsTab)
                        Factory.GUIController.SelectEventsTabInPropertyGrid();
                }
                else
                {
                    Factory.GUIController.ShowMessage($"There was error trying to open an object {objectName} of type {typeName} for editing.", MessageBoxIcon.Information);
                }
            }
            else
            {
                Factory.GUIController.ShowMessage("This symbol is internally defined by AGS and probably corresponds to an in-game entity which does not support \"Go to\" at the moment.", MessageBoxIcon.Information);
            }
        }

        public void ZoomToFile(string fileName)
        {
            ZoomToFile(fileName, ZoomToFileZoomType.DoNotMoveCursor, 0, false);
        }

        public void ZoomToFile(string fileName, int lineNumber)
        {
            ZoomToFile(fileName, ZoomToFileZoomType.ZoomToLineNumber, lineNumber, false);
        }

        public void ZoomToFile(string fileName, ZoomToFileZoomType zoomType, int zoomToPosition)
		{
			ZoomToFile(fileName, zoomType, zoomToPosition, false);
		}

		public void ZoomToFile(string fileName, int lineNumber, bool isDebugExecutionPoint, string errorMessage)
		{
			ZoomToFile(fileName, ZoomToFileZoomType.ZoomToLineNumber, lineNumber, isDebugExecutionPoint, false, errorMessage, true);
		}

		public void ZoomToFile(string fileName, ZoomToFileZoomType zoomType, int zoomPosition, bool isDebugExecutionPoint)
		{
			ZoomToFile(fileName, zoomType, zoomPosition, isDebugExecutionPoint, true, null, true);
		}

        public void ZoomToFile(string fileName, ZoomToFileZoomType zoomType, int zoomPosition, bool isDebugExecutionPoint, bool selectWholeLine, string errorMessage, bool activateEditor)
        {
            if (OnZoomToFile != null)
            {
                if (this.InvokeRequired)
                {
					this.Invoke(new ZoomToFileDelegate(ZoomToFile), fileName, zoomType, zoomPosition, isDebugExecutionPoint, selectWholeLine, errorMessage, activateEditor);
                }
                else
                {
					ZoomToFileEventArgs evArgs = new ZoomToFileEventArgs(fileName, zoomType, ZoomToFileMatchStyle.MatchExact, zoomPosition, null, isDebugExecutionPoint, errorMessage, activateEditor);
					evArgs.SelectLine = selectWholeLine;
					OnZoomToFile(evArgs);
                }
            }
        }

        /// <summary>
        /// Zooms to a function definition in the given script file.
        /// Function is expected to be either of "void" or "function" type.
        /// NOTE: this method is intended for event callbacks which do not normally have a return value (so no return type).
        /// </summary>
        public void ZoomToFile(string fileName, string function)
        {
            if (OnZoomToFile != null)
            {
                string pattern = string.Format(ScriptGeneration.SCRIPT_EVENT_FUNCTION_PATTERN, function);
                ZoomToFileEventArgs evArgs = new ZoomToFileEventArgs(fileName, ZoomToFileZoomType.ZoomToText, ZoomToFileMatchStyle.MatchRegex, pattern);
                evArgs.SelectLine = false;
                evArgs.ZoomToLineAfterOpeningBrace = true;
                OnZoomToFile(evArgs);
            }
        }

        public IScriptEditorControl GetScriptEditorControl(string scriptFileName, bool showEditor)
        {
            if (OnGetScriptEditorControl != null)
            {
                GetScriptEditorControlEventArgs evArgs = new GetScriptEditorControlEventArgs(scriptFileName, showEditor);
                OnGetScriptEditorControl(evArgs);
                return evArgs.ScriptEditor;
            }

            return null;
        }

        public void DocumentTitlesChanged()
        {
            _mainForm.DocumentTitlesChanged();
        }

        public void SetPropertyGridObjectList(Dictionary<string, object> propertyObjects)
        {
            if (_mainForm.ActivePane != null)
            {
                _mainForm.ActivePane.PropertyGridObjectList = propertyObjects;
                _mainForm.RefreshPropertyGridForDocument(_mainForm.ActivePane);
            }
        }

        /// <summary>
        /// Sets property list for the given ContentDocument, and displays it in the
        /// property grid if the given document is an active pane
        /// (otherwise just assign them to the given document).
        /// Optionally provides default object: that object will be selected if
        /// previously selected object is no longer available in the new list.
        /// </summary>
        public void SetPropertyGridObjectList(Dictionary<string, object> propertyObjects, ContentDocument doc, object defObject)
        {
            if (_mainForm.ActivePane == doc)
            {
                _mainForm.ActivePane.PropertyGridObjectList = propertyObjects;
                _mainForm.RefreshPropertyGridForDocument(_mainForm.ActivePane);
            }
            else
            {
                object selObject = doc.SelectedPropertyGridObject;
                doc.PropertyGridObjectList = propertyObjects;
                if (!propertyObjects.ContainsValue(selObject))
                    doc.SelectedPropertyGridObject = defObject;
            }
        }

        public object GetPropertyGridObject()
        {
            if (_mainForm.ActivePane == null) return null;
            return _mainForm.ActivePane.SelectedPropertyGridObject;
        }

        public object[] GetPropertyGridObjects()
        {
            if (_mainForm.ActivePane == null) return null;
            return _mainForm.ActivePane.SelectedPropertyGridObjects;
        }

        public void SetPropertyGridObject(object objectToSetPropertiesOn)
        {
            if (_mainForm.ActivePane != null)
            {
                _mainForm.ActivePane.SelectedPropertyGridObject = objectToSetPropertiesOn;
                _mainForm.ActivePane.SelectedPropertyGridObjects = null;
                _mainForm.RefreshPropertyGridForDocument(_mainForm.ActivePane);
            }
        }

        /// <summary>
        /// Set selected property object for the given ContentDocument, and displays it in the
        /// property grid if the given document is an active pane
        /// (otherwise just assign them to the given document).
        /// </summary>
        public void SetPropertyGridObject(object objectToSetPropertiesOn, ContentDocument doc)
        {
            if (_mainForm.ActivePane == doc)
            {
                _mainForm.ActivePane.SelectedPropertyGridObject = objectToSetPropertiesOn;
                _mainForm.ActivePane.SelectedPropertyGridObjects = null;
                _mainForm.RefreshPropertyGridForDocument(_mainForm.ActivePane);
            }
            else
            {
                doc.SelectedPropertyGridObject = objectToSetPropertiesOn;
            }
        }

        public void SetPropertyGridObjects(object[] objectsToSetPropertiesOn)
        {
            if (_mainForm.ActivePane != null)
            {
                _mainForm.ActivePane.SelectedPropertyGridObjects = objectsToSetPropertiesOn;
                _mainForm.ActivePane.SelectedPropertyGridObject = null;
                _mainForm.RefreshPropertyGridForDocument(_mainForm.ActivePane);
            }
        }

        public void RefreshPropertyGrid()
        {
            _mainForm.RefreshPropertyGridForDocument(_mainForm.ActivePane);
        }

        public void SelectPropertyByName(string propertyName)
        {
            _mainForm.SelectPropertyByName(propertyName);
        }

        public bool SelectEventsTabInPropertyGrid()
        {
            return _mainForm.SelectTabInPropertyGrid("Events");
        }

        public void MoveMouseCursorToPropertyGrid()
        {
            Point gridPosition = _mainForm.GetPropertyGridScreenCoordinates();
            Cursor.Position = new Point(gridPosition.X + 90, gridPosition.Y + 30);
        }

        public string ShowSaveFileDialog(string title, string fileFilter, string initialDirectory = null)
        {
            SaveFileDialog dialog = new SaveFileDialog();
            dialog.Title = title;
            dialog.RestoreDirectory = true;
            dialog.OverwritePrompt = true;
            dialog.ValidateNames = true;
            dialog.InitialDirectory = initialDirectory == null ? Directory.GetCurrentDirectory() : initialDirectory;
            dialog.Filter = fileFilter;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                return dialog.FileName;
            }
            return null;
        }

		public string ShowOpenFileDialog(string title, string fileFilter)
		{
			return ShowOpenFileDialog(title, fileFilter, true);
		}

		private void EnsureLastImportDirectoryIsSet(bool useFileImportPath)
		{
			if (_lastImportDirectory == null)
			{
				if ((useFileImportPath) &&
					(Factory.AGSEditor.Settings.DefaultImportPath.Length > 0))
				{
					_lastImportDirectory = Factory.AGSEditor.Settings.DefaultImportPath;
				}
				else
				{
					_lastImportDirectory = System.IO.Directory.GetCurrentDirectory();
				}
			}
		}

        public string ShowOpenFileDialog(string title, string fileFilter, string initialDirectory)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Title = title;
            dialog.RestoreDirectory = true;
            dialog.CheckFileExists = true;
            dialog.CheckPathExists = true;
            dialog.InitialDirectory = initialDirectory == null ? Directory.GetCurrentDirectory() : initialDirectory;
            dialog.ValidateNames = true;
            dialog.Filter = fileFilter;

            if (dialog.ShowDialog() == DialogResult.OK)
            {
                _lastImportDirectory = Path.GetDirectoryName(dialog.FileName);
                return dialog.FileName;
            }
            return null;
        }

        public string ShowOpenFileDialog(string title, string fileFilter, bool useFileImportPath)
        {
			EnsureLastImportDirectoryIsSet(useFileImportPath);

            return ShowOpenFileDialog(title, fileFilter, _lastImportDirectory);
        }

        public string[] ShowOpenFileDialogMultipleFiles(string title, string fileFilter)
        {
            EnsureLastImportDirectoryIsSet(true);

            return ShowOpenFileDialogMultipleFiles(title, fileFilter, _lastImportDirectory);
        }

        public string[] ShowOpenFileDialogMultipleFiles(string title, string fileFilter, string initialDirectory)
        {
            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Title = title;
            dialog.RestoreDirectory = true;
            dialog.CheckFileExists = true;
            dialog.CheckPathExists = true;
            dialog.InitialDirectory = initialDirectory;
            dialog.ValidateNames = true;
            dialog.Filter = fileFilter;
            dialog.Multiselect = true;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
				_lastImportDirectory = Path.GetDirectoryName(dialog.FileNames[0]);
                return dialog.FileNames;
            }
            return new string[0];
        }

        public string ShowSelectFolderOrNoneDialog(string title, string initialPath, bool allowNewFolder)
        {
            FolderBrowserDialog dialog = new FolderBrowserDialog();
            dialog.Description = title;
            dialog.SelectedPath = initialPath;
            dialog.ShowNewFolderButton = allowNewFolder;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                return dialog.SelectedPath;
            }
            return null;
        }

        public string ShowSelectFolderOrNoneDialog(string title, string initialPath)
        {
            return ShowSelectFolderOrNoneDialog(title, initialPath, true);
        }

        public string ShowSelectFolderOrDefaultDialog(string title, string defaultPath, bool allowNewFolder)
        {
            string result = defaultPath;
            FolderBrowserDialog dialog = new FolderBrowserDialog();
            dialog.Description = title;
            dialog.ShowNewFolderButton = allowNewFolder;
            if (defaultPath.Length > 0)
            {
                dialog.SelectedPath = defaultPath;
            }
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                result = dialog.SelectedPath;
            }
            return result;
        }

        public string ShowSelectFolderOrDefaultDialog(string title, string defaultPath)
        {
            return ShowSelectFolderOrDefaultDialog(title, defaultPath, true);
        }

        public int ShowChangeObjectIDDialog(string objectType, int oldValue, int minValue, int maxValue)
        {
            return NumberEntryWithInfoDialog.Show(string.Format("Change {0} ID", objectType),
                string.Format("Enter the new ID in the box below ({0}-{1}):", minValue, maxValue),
                "WARNING: Changing the game item's ID is a specialized operation, for advanced users only. It is meant for rearranging item order inside their list. Thus changing IDs works as swapping position of two items.\n\nPlease note that this will affect any script that refers to items by their number rather than script name.",
                oldValue, minValue, maxValue);
        }

        public int ShowChangeRoomNumberDialog(int oldValue)
        {
            NewRoomDialog dialog = new NewRoomDialog(_agsEditor.CurrentGame, oldValue);
            if (dialog.ShowDialog() == DialogResult.OK)
                return dialog.ChosenRoomNumber;
            return oldValue;
        }

        public void Initialize(AGSEditor agsEditor)
        {
            if (_mainForm == null)
            {
                _agsEditor = agsEditor;
                _interactiveTasks = new InteractiveTasks(_agsEditor.Tasks);
                ColorThemes = new ColorThemes(_agsEditor, _agsEditor.Settings);
                WindowConfig = new ObjectConfigJson();
                _mainForm = new frmMain();
                SetEditorWindowSize();
                _treeManager = new ProjectTree(_mainForm.projectPanel.projectTree);
                _treeManager.OnContextMenuClick += _mainForm_OnMenuClick;
                _toolBarManager = new ToolBarManager(_mainForm.toolStrip);
                _windowsMenuManager = new WindowsMenuManager(_mainForm.windowsToolStripMenuItem, 
                    _mainForm.DockPanes, _mainForm.mainContainer, _mainForm.GetLayoutManager());
                _menuManager = new MainMenuManager(_mainForm.mainMenu, _windowsMenuManager);
                _mainForm.OnEditorShutdown += _mainForm_OnEditorShutdown;
                _mainForm.OnPropertyChanged += _mainForm_OnPropertyChanged;
                _mainForm.OnPropertyObjectChanged += _mainForm_OnPropertyObjectChanged;
                _mainForm.OnActiveDocumentChanged += _mainForm_OnActiveDocumentChanged;
                _mainForm.OnMainWindowActivated += _mainForm_OnMainWindowActivated;
                _menuManager.OnMenuClick += _mainForm_OnMenuClick;
                AutoComplete.BackgroundCacheUpdateStatusChanged += AutoComplete_BackgroundCacheUpdateStatusChanged;
				SystemEvents.DisplaySettingsChanged += SystemEvents_DisplaySettingsChanging;

                RegisterIcon("SpriteIcon", Resources.ResourceManager.GetIcon("iconspr.ico"));
                RegisterIcon("BuildIcon", Resources.ResourceManager.GetIcon("menu_build_rebuild-files.ico"));
                RegisterIcon("GameIcon", Resources.ResourceManager.GetIcon("game.ico"));
				RegisterIcon("CompileErrorIcon", Resources.ResourceManager.GetIcon("eventlogError.ico"));
				RegisterIcon("CompileWarningIcon", Resources.ResourceManager.GetIcon("eventlogWarn.ico"));
                RegisterIcon("OpenContainingFolderIcon", Resources.ResourceManager.GetIcon("menu_file_openfolder.ico"));
                _mainForm.SetTreeImageList(_imageList);
                _mainForm.mainMenu.ImageList = _imageList;
				_mainForm.pnlOutput.SetImageList(_imageList);

                ViewUIEditor.ViewSelectionGUI = ShowViewChooserFromPropertyGrid;
                SpriteSelectUIEditor.SpriteSelectionGUI = ShowSpriteChooserFromPropertyGrid;
                CustomPropertiesUIEditor.CustomPropertiesGUI = ShowPropertiesEditorFromPropertyGrid;
                PropertyTabInteractions.UpdateEventName = PropertyTabInteractions_UpdateEventName;
                ScriptFunctionUIEditor.OpenScriptFunction = ScriptFunctionUIEditor_OpenScriptFunction;
                ScriptFunctionUIEditor.CreateScriptFunction = ScriptFunctionUIEditor_CreateScriptFunction;
                CustomResolutionUIEditor.CustomResolutionSetGUI = ShowCustomResolutionChooserFromPropertyGrid;
                ColorUIEditor.ColorGUI = ShowColorDialog;
                MultiLineStringUIEditor.MultilineStringGUI = ShowMultilineStringDialog;
                AudioClipSourceFileUIEditor.AudioClipSourceFileGUI = ShowAudioClipSourceFileChooserFromPropertyGrid;
            }
        }

        private void _mainForm_OnMainWindowActivated(object sender, EventArgs e)
        {
            if (OnMainWindowActivated != null)
            {
                OnMainWindowActivated(this, e);
            }
        }

		private void SystemEvents_DisplaySettingsChanging(object sender, EventArgs e)
		{
			_mainForm.SetDrawingSuspended(SystemInformation.PrimaryMonitorSize.Width < 800);
		}

        private void Events_GamePostLoad(Game game)
        {
            ClearOutputPanel();
            ClearCallStack();
            ClearFindSymbolResults();
            ClearEngineLogMessages();
        }

        public void ExitApplication()
        {
            _mainForm.Close();
        }

        /// <summary>
        /// Loads Window configuration from the given file.
        /// NOTE: this does not apply configuration to windows immediately,
        /// this only loads configuration into the memory, where it may be accessed
        /// by any window on demand. For this reason this config should be loaded
        /// as early in Editor's session as possible.
        /// </summary>
        public bool LoadWindowConfig()
        {
            return WindowConfig.LoadFromFile(Path.Combine(Factory.AGSEditor.LocalAppData, WINDOW_CONFIG_FILENAME));
        }

        /// <summary>
        /// Saves Window configuration into the given file. 
        /// </summary>
        public void SaveWindowConfig()
        {
            WindowConfig.SaveToFile(Path.Combine(Factory.AGSEditor.LocalAppData, WINDOW_CONFIG_FILENAME));
        }

        public void CompileAndExit(string projectPath)
        {
            bool error = true;
            if (_interactiveTasks.LoadGameFromDisk(projectPath))
            {
                error = false;
                bool forceRebuild = _agsEditor.NeedsRebuildForDebugMode();
                var messages = _agsEditor.CompileGame(forceRebuild, false);
                if (forceRebuild)
                    _agsEditor.SaveUserDataFile(); // in case pending config is applied

                _batchProcessShutdown = true;
                if (messages.Count == 0)
                {
                    BuildCommandsComponent.ShowCompileSuccessMessage();
                }
                else
                {
                    error = true;
                }
            }
            if(error) Program.SetExitCode(1);
            this.ExitApplication();
        }

        public void SaveAsTemplateAndExit(string projectPath)
        {
            bool error = true;
            if (_interactiveTasks.LoadGameFromDisk(projectPath))
            {
                error = false;
                bool forceRebuild = _agsEditor.NeedsRebuildForDebugMode();
                _agsEditor.CompileGame(forceRebuild, false);
                if (forceRebuild)
                    _agsEditor.SaveUserDataFile(); // in case pending config is applied

                _batchProcessShutdown = true;

                error = !SaveGameAsTemplate(_agsEditor.CurrentGame.Settings.GameFileName + ".agt");
            }
            if (error) Program.SetExitCode(1);
            this.ExitApplication();
        }

        // TODO: ShowWelcomeScreen has a return value but it's never checked;
        // figure out if it needs one and what it's meaning is
        public bool ShowWelcomeScreen()
        {
            if (AGS.Types.Version.IS_BETA_VERSION)
            {
                Factory.GUIController.ShowMessage("This is a BETA version of AGS. BE VERY CAREFUL and MAKE SURE YOU BACKUP YOUR GAME before loading it in this editor.", MessageBoxIcon.Warning);
            }

            bool projectExists = !string.IsNullOrEmpty(_commandOptions.ProjectPath);
            if (string.IsNullOrEmpty(_commandOptions.ProjectPath))
            {
                if (_commandOptions.AutoOperationRequested)
                {
                    Factory.GUIController.ShowMessage("Unable to perform a requested operation, no project path was provided", MessageBoxIcon.Warning);
                }
            }
            else if (!File.Exists(_commandOptions.ProjectPath))
            {
                Factory.GUIController.ShowMessage($"Unable to load the game '{_commandOptions.ProjectPath}' because it does not exist", MessageBoxIcon.Warning);
                projectExists = false;
            }

            bool showWelcomeScreen = false;
            if (projectExists)
            {
                if (_commandOptions.CompileAndExit)
                {
                    CompileAndExit(_commandOptions.ProjectPath);
                }
                else if (_commandOptions.TemplateSaveAndExit)
                {
                    SaveAsTemplateAndExit(_commandOptions.ProjectPath);
                }
                else if (!string.IsNullOrEmpty(_commandOptions.ProjectPath))
                {
                    _interactiveTasks.LoadGameFromDisk(_commandOptions.ProjectPath);
                }
            }
            else
            {
                // If we were asked to perform certain action over a project,
                // but the project cannot be loaded, then exit right away.
                if (_commandOptions.AutoOperationRequested)
                {
                    _exitFromWelcomeScreen = true;
                    Application.Exit();
                    return false;
                }

                showWelcomeScreen = true;
            }

            while (showWelcomeScreen)
            {
				Directory.SetCurrentDirectory(_agsEditor.EditorDirectory);
                WelcomeScreen welcomeScreen = new WelcomeScreen();
				DialogResult result = welcomeScreen.ShowDialog();
                WelcomeScreenSelection selection = welcomeScreen.SelectedOption;
                showWelcomeScreen = false;

                if (result == DialogResult.Cancel)
                {
                    _exitFromWelcomeScreen = true;
                    Application.Exit();
                }
                else if (selection == WelcomeScreenSelection.LoadExistingGame)
                {
                    if (!_interactiveTasks.BrowseForAndLoadGame())
                    {
                        showWelcomeScreen = true;
                    }
                }
                else if (selection == WelcomeScreenSelection.ContinueRecentGame)
                {
                    string gamePath = welcomeScreen.GetSelectedRecentGamePath();
                    string gameToLoad = Path.Combine(gamePath, AGSEditor.GAME_FILE_NAME);
                    if (!File.Exists(gameToLoad))
                    {
                        Factory.GUIController.ShowMessage("Unable to find a valid game file in " + gamePath, MessageBoxIcon.Warning);
                        showWelcomeScreen = true;
                    }
                    else if (!_interactiveTasks.LoadGameFromDisk(gameToLoad))
                    {
                        showWelcomeScreen = true;
                    }
                }
                else
                {
                    showWelcomeScreen = !ShowNewGameWizard();
                }

                welcomeScreen.Dispose();
            }
			return _exitFromWelcomeScreen;
        }

        private bool ShowNewGameWizard()
        {
            bool createdSuccessfully = false;
            List<GameTemplate> templates = new List<GameTemplate>();
            string[] directories = new string[] { _agsEditor.TemplatesDirectory, _agsEditor.UserTemplatesDirectory };

            foreach (string directory in directories)
            {
                foreach (string fileName in Utilities.GetDirectoryFileList(directory, "*.agt"))
                {
                    GameTemplate template = Factory.NativeProxy.LoadTemplateFile(fileName);
                    if (template != null)
                    {
                        templates.Add(template);
                    }
                }
            }

            string newGamePath;

            if (String.IsNullOrWhiteSpace(Factory.AGSEditor.Settings.NewGamePath))
            {
                newGamePath = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
            }
            else
            {
                newGamePath = Factory.AGSEditor.Settings.NewGamePath;
            }

            List<WizardPage> pages = new List<WizardPage>();
            StartNewGameTemplatesPage templateSelectPage = new StartNewGameTemplatesPage(templates);
            StartNewGameDetailsPage gameNameSelectPage = new StartNewGameDetailsPage(newGamePath);
            pages.Add(templateSelectPage);
            pages.Add(gameNameSelectPage);
            
            WizardDialog dialog = new WizardDialog("Start New Game", "This wizard will guide you through the process of creating a new game.", pages);
            DialogResult result = dialog.ShowDialog();

            if (result == DialogResult.OK)
            {
                createdSuccessfully = CreateNewGame(gameNameSelectPage.GetFullPath(), gameNameSelectPage.FileName,
                    gameNameSelectPage.NewGameName, templateSelectPage.SelectedTemplate);
            }

            dialog.Dispose();
            return createdSuccessfully;
        }

        private bool CreateNewGame(string newGamePath, string newFileName, string newGameName, GameTemplate createFromTemplate)
        {
            bool createdSuccessfully = false;
            try
            {
                string templateFileName = Path.Combine(_agsEditor.EditorDirectory, createFromTemplate.FileName);
                _agsEditor.Tasks.CreateNewGameFromTemplate(templateFileName, newGamePath);
                string newGameFileName = Path.Combine(newGamePath, AGSEditor.GAME_FILE_NAME);
                if (_agsEditor.Tasks.LoadGameFromDisk(newGameFileName, false))
                {
                    _agsEditor.CurrentGame.Settings.GameFileName = newFileName;
                    _agsEditor.CurrentGame.Settings.GameName = newGameName;
                    _agsEditor.CurrentGame.Settings.SaveGameFolderName = newGameName;
                    _agsEditor.CurrentGame.Settings.GenerateNewGameID();
                    _agsEditor.CurrentGame.DefaultSetup.TitleText = _agsEditor.CurrentGame.Settings.GameName + " Setup";
                    Factory.GUIController.GameNameUpdated();
                    _agsEditor.CurrentGame.WorkspaceState.LastBuildConfiguration = _agsEditor.CurrentGame.Settings.DebugMode ? BuildConfiguration.Debug : BuildConfiguration.Release;
                    if (_agsEditor.SaveGameFiles())
                    {
                        // TODO: seriously, running whole game compilation is
                        // not a good solution for updating room files......
                        // change this to do exactly what's necessary and not
                        // building all default build targets!

                        // Force a rebuild to remove the key in the room
                        // files that links them to the old game ID
                        // Force no message to be displayed if the build fails
                        // (which it will for the Empty Game template)
                        MessageBoxOnCompile oldMessageBoxSetting = Factory.AGSEditor.Settings.MessageBoxOnCompile;
                        Factory.AGSEditor.Settings.MessageBoxOnCompile = MessageBoxOnCompile.Never;

                        _agsEditor.CompileGame(true, false);
                        // The user data may have been amended by the building process
                        _agsEditor.SaveUserDataFile();

                        Factory.AGSEditor.Settings.MessageBoxOnCompile = oldMessageBoxSetting;
                    }
                    createdSuccessfully = true;
                }
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was an error creating your game. The error was: " + ex.Message + Environment.NewLine + Environment.NewLine + "Error details: " + ex.ToString(), MessageBoxIcon.Stop);
            }

            if (!createdSuccessfully)
            {
                Directory.SetCurrentDirectory(_agsEditor.EditorDirectory);
                if (Directory.Exists(newGamePath))
                {
                    Directory.Delete(newGamePath, true);
                }
            }

            return createdSuccessfully;
        }

		private string[] ConstructRoomTemplateFileList(UnloadedRoom room)
		{
			List<string> files = new List<string>();
			Utilities.AddAllMatchingFiles(files, Path.ChangeExtension(room.FileName, ".ico"));
			Utilities.AddAllMatchingFiles(files, ROOM_TEMPLATE_ID_FILE);
			Utilities.AddAllMatchingFiles(files, room.FileName);
			Utilities.AddAllMatchingFiles(files, room.ScriptFileName);
			return files.ToArray();
		}

        public void SaveRoomAsTemplate(UnloadedRoom room)
        {
            string filename = Factory.GUIController.ShowSaveFileDialog("Save room template as...", Constants.ROOM_TEMPLATE_FILE_FILTER, Factory.AGSEditor.UserTemplatesDirectory);

            if (filename != null)
            {
                try
                {
                    BinaryWriter writer = new BinaryWriter(new FileStream(ROOM_TEMPLATE_ID_FILE, FileMode.Create, FileAccess.Write));
                    writer.Write(ROOM_TEMPLATE_ID_FILE_SIGNATURE);
                    writer.Write(room.Number);
                    writer.Close();

                    Factory.NativeProxy.CreateTemplateFile(filename, ConstructRoomTemplateFileList(room));
                    Utilities.TryDeleteFile(ROOM_TEMPLATE_ID_FILE);
                }
                catch (AGSEditorException ex)
                {
                    Factory.GUIController.ShowMessage("There was an error creating your room template:" + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
                }
                catch (Exception ex)
                {
                    Factory.GUIController.ShowMessage("There was an error creating your room template. The error was: " + ex.Message + Environment.NewLine + Environment.NewLine + "Error details: " + ex.ToString(), MessageBoxIcon.Warning);
                }
            }
        }

        private bool SaveGameAsTemplate(string filename)
        {
            bool success = false;
            try
            {
                _agsEditor.SaveGameFiles();
                InteractiveTasks.CreateTemplateFromCurrentGame(filename);
                success = true;
            }
            catch (AGSEditorException ex)
            {
                Factory.GUIController.ShowMessage("There was an error creating your template:" + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
                success = false;
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was an error creating your template. The error was: " + ex.Message + Environment.NewLine + Environment.NewLine + "Error details: " + ex.ToString(), MessageBoxIcon.Warning);
                success = false;
            }
            return success;
        }

        public void SaveGameAsTemplate()
        {
            string filename = Factory.GUIController.ShowSaveFileDialog("Save new template as...", Constants.GAME_TEMPLATE_FILE_FILTER, Factory.AGSEditor.UserTemplatesDirectory);

            if (filename != null)
            {
                SaveGameAsTemplate(filename);
            }
        }

		public void ShowCreateVoiceActingScriptWizard()
		{
			List<WizardPage> pages = new List<WizardPage>();
			CreateVoiceActingScriptPage mainPage = new CreateVoiceActingScriptPage();
			mainPage.SelectedFilePath = Path.Combine(_agsEditor.CurrentGame.DirectoryPath, "VoiceSpeech.txt");
			pages.Add(mainPage);

			WizardDialog dialog = new WizardDialog("Voice Acting Script", "This wizard will guide you through creating a voice acting script. This script is a text file that you can supply to your voice actors containing a list of lines that they need to record.", pages);
			DialogResult result = dialog.ShowDialog();

			if (result == DialogResult.OK)
			{
				try
				{
					_agsEditor.SaveGameFiles();
					CompileMessages results = (CompileMessages)BusyDialog.Show("Please wait while your script is created...", new BusyDialog.ProcessingHandler(CreateVoiceActingScriptProcess), mainPage.SelectedFilePath);
					Factory.GUIController.ShowOutputPanel(results);
					if (results.HasErrors)
					{
						Factory.GUIController.ShowMessage("There were errors creating the voice acting script. See the output window for details.", MessageBoxIcon.Warning);
					}
					else
					{
						Factory.GUIController.ShowMessage("Voice acting script created successfully.", MessageBoxIcon.Information);
					}
				}
				catch (Exception ex)
				{
					Factory.GUIController.ShowMessage("There was an error creating the voice acting script. The error was: " + ex.Message + Environment.NewLine + Environment.NewLine + "Error details: " + ex.ToString(), MessageBoxIcon.Warning);
				}
			}

			dialog.Dispose();
		}

        public void ShowAutoNumberSpeechWizard()
        {
            List<WizardPage> pages = new List<WizardPage>();
            AutoNumberSpeechWizardPage mainPage = new AutoNumberSpeechWizardPage();
            pages.Add(mainPage);
            AutoNumberSpeechWizardPage2 advancedOptions = new AutoNumberSpeechWizardPage2(_agsEditor.CurrentGame.RootCharacterFolder.AllItemsFlat);
            pages.Add(advancedOptions);

            WizardDialog dialog = new WizardDialog("Number Speech Lines", "This wizard will guide you through automatically numbering your game speech lines.\n\nThis process automatically assigns speech numbers to all displayable text in the game. It will modify your dialogs and scripts in order to assign a unique number to each line of text for each character.\n\nWARNING: It is STRONGLY RECOMMENDED that you back up your game before continuing, just in case anything goes wrong.\n\nWARNING: Running this process will overwrite any existing speech lines and re-number them all from scratch.", pages);
            DialogResult result = dialog.ShowDialog();

            if (result == DialogResult.OK)
            {
                AutoNumberSpeechOptions options = AutoNumberSpeechOptions.None;
                if (mainPage.EnableNarrator) options |= AutoNumberSpeechOptions.DoNarrator;
                if (mainPage.CombineIdenticalLines) options |= AutoNumberSpeechOptions.CombineIdenticalLines;
                if (advancedOptions.RemoveNumbering) options |= AutoNumberSpeechOptions.RemoveNumbering;
                AutoNumberSpeechData data = new AutoNumberSpeechData();
                data.Options = options;
                data.CharacterID = advancedOptions.SelectedCharacterID;
                
                try
                {
                    _agsEditor.SaveGameFiles();
                    CompileMessages results = (CompileMessages)BusyDialog.Show("Please wait while your speech lines are numbered...", new BusyDialog.ProcessingHandler(AutoNumberSpeechLinesProcess), data);
                    Factory.GUIController.ShowOutputPanel(results);
                    Factory.Events.OnRefreshAllComponentsFromGame();
                    _agsEditor.SaveGameFiles(); 
                    Factory.GUIController.ShowMessage("Speech lines numbered successfully.", MessageBoxIcon.Information);
                }
                catch (Exception ex)
                {
                    Factory.GUIController.ShowMessage("There was an error numbering your speech lines. The error was: " + ex.Message + Environment.NewLine + Environment.NewLine + "Error details: " + ex.ToString(), MessageBoxIcon.Warning);
                }
            }

            dialog.Dispose();
        }

        private object AutoNumberSpeechLinesProcess(IWorkProgress progress, object parameter)
        {
            AutoNumberSpeechData data = (AutoNumberSpeechData)parameter;
            bool doNarrator = (data.Options & AutoNumberSpeechOptions.DoNarrator) != 0;
            bool combineIdenticalLines = (data.Options & AutoNumberSpeechOptions.CombineIdenticalLines) != 0;
            bool removeNumbering = (data.Options & AutoNumberSpeechOptions.RemoveNumbering) != 0;
            return new SpeechLinesNumbering().NumberSpeechLines(_agsEditor.CurrentGame, doNarrator, combineIdenticalLines, removeNumbering, data.CharacterID);
        }

		private object CreateVoiceActingScriptProcess(IWorkProgress progress, object parameter)
		{
			string outputFile = (string)parameter;
			VoiceActorScriptGenerator generator = new VoiceActorScriptGenerator();
			CompileMessages results = generator.CreateVoiceActingScript(_agsEditor.CurrentGame);
			if (!results.HasErrors)
			{
				if (_agsEditor.AttemptToGetWriteAccess(outputFile))
				{
					using (StreamWriter sw = new StreamWriter(outputFile, false))
					{
						foreach (int charID in generator.LinesByCharacter.Keys)
						{
							WriteLinesForCharacter(charID, sw, generator.LinesByCharacter[charID]);
						}

						sw.WriteLine();
						sw.WriteLine("*** All text lines, in order of appearance in the scripts ***");
						sw.WriteLine();

						foreach (GameTextLine line in generator.LinesInOrder)
						{
							Character character = _agsEditor.CurrentGame.FindCharacterByID(line.CharacterID);
							string characterName = "NARRATOR";
							if (character != null)
							{
								characterName = character.ScriptName;
							}
							WriteLineIfItHasVoiceFile(characterName, line.Text, sw);
						}
					}
				}
			}
			return results;
		}

		private void WriteLinesForCharacter(int charID, StreamWriter sw, Dictionary<string,string> lines)
		{
			Character character = _agsEditor.CurrentGame.FindCharacterByID(charID);
			if (character != null)
			{
				sw.WriteLine();
				sw.WriteLine("*** Lines for character " + character.RealName + " (ID " + charID + ") ***");
				sw.WriteLine();
			}
			else
			{
				sw.WriteLine();
				sw.WriteLine("*** Lines for narrator ***");
				sw.WriteLine();
			}

			foreach (string line in lines.Keys)
			{
				WriteLineIfItHasVoiceFile(string.Empty, line, sw);
			}
		}

		private void WriteLineIfItHasVoiceFile(string prefix, string line, StreamWriter sw)
		{
			if (line.StartsWith("&"))
			{
				sw.WriteLine(prefix + ((prefix.Length > 0) ? ": " : string.Empty) + line);
			}
		}

		public void ShowPreferencesEditor()
        {
            PreferencesEditor prefsEditor = new PreferencesEditor();
            if (prefsEditor.ShowDialog() == DialogResult.OK)
            {
                Factory.AGSEditor.Settings.Save();
				//_mainForm.SetProjectTreeLocation(_agsEditor.Preferences.ProjectTreeOnRight);
            }
            prefsEditor.Dispose();
        }

        private bool ScriptFunctionUIEditor_CreateScriptFunction(CreateScriptFunctionArgs args)
        {
            if (OnGetScript == null)
                return false;

            Script script = null;
            OnGetScript(args.ScriptName, ref script);
            if (script == null)
                return false;

            bool allowEdit = true;
            AttemptToEditScript?.Invoke(ref allowEdit);
            if (!allowEdit)
                return false;

            if (_agsEditor.AttemptToGetWriteAccess(script.FileName))
            {
                script.Text = ScriptGeneration.InsertFunction(script.Text, args.FunctionName, args.FunctionParameters);
                if (script.Modified)
                    OnScriptChanged?.Invoke(script);
                return true;
            }

            return false;
        }

        private void ScriptFunctionUIEditor_OpenScriptFunction(OpenScriptFunctionArgs args)
        {
            if (OnZoomToFile != null)
            {
                // We need to start a timer, because we are within the
                // property grid processing at the moment
                string searchForText = string.Format(ScriptGeneration.SCRIPT_EVENT_FUNCTION_PATTERN, args.FunctionName);
                TickOnceTimer.CreateAndStart(100, new EventHandler((sender, e) => ZoomToScriptFunction(args, searchForText, ZoomToFileMatchStyle.MatchRegex)));
            }
        }

        private void ZoomToScriptFunction(OpenScriptFunctionArgs openArgs, string searchForText, ZoomToFileMatchStyle matchStyle)
        {
            ZoomToFileEventArgs evArgs = new ZoomToFileEventArgs(openArgs.ScriptName, ZoomToFileZoomType.ZoomToText, matchStyle, searchForText);
			evArgs.SelectLine = false;
            evArgs.ZoomToLineAfterOpeningBrace = true;
			OnZoomToFile(evArgs);

            if (evArgs.Result == ZoomToFileResult.LocationNotFound &&
                openArgs.CreateIfNotExists)
            {
                // Create, reset args, and try to zoom in once more
                ScriptFunctionUIEditor_CreateScriptFunction(openArgs);
                evArgs = new ZoomToFileEventArgs(openArgs.ScriptName, ZoomToFileZoomType.ZoomToText, matchStyle, searchForText);
                evArgs.SelectLine = false;
                evArgs.ZoomToLineAfterOpeningBrace = true;
                OnZoomToFile(evArgs);
            }
        }

        private string PropertyTabInteractions_UpdateEventName(string eventName)
        {
            Game game = _agsEditor.CurrentGame;
            for (int i = 1; i <= Math.Min(9, game.Cursors.Count - 1); i++)
            {
                eventName = eventName.Replace("$$0" + i, game.Cursors[i].Name);
            }
            return eventName;
        }

        private Size ShowCustomResolutionChooserFromPropertyGrid(Size currentSize)
        {
            return CustomResolutionDialog.Show(currentSize);
        }

        private String ShowMultilineStringDialog(string title, String text)
        {
            return MultilineStringEditorDialog.ShowEditor(title, text ?? string.Empty);
        }

        private Color? ShowColorDialog(Color? color)
        {
            ColorDialog dialog = new ColorDialog();
            dialog.Color = color ?? Color.White;
            dialog.FullOpen = true;
            dialog.SolidColorOnly = true;
            dialog.CustomColors = _customColors;
            bool res = dialog.ShowDialog() == DialogResult.OK;
            _customColors = (int[])dialog.CustomColors.Clone();
            return res ? dialog.Color : (Color?)null;
        }

        private void ShowPropertiesEditorFromPropertyGrid(CustomProperties props, object objectThatHasProperties)
        {
            CustomPropertyAppliesTo propertyTypes;
            if (objectThatHasProperties is Character)
            {
                propertyTypes = CustomPropertyAppliesTo.Characters;
            }
            else if (objectThatHasProperties is Dialog)
            {
                propertyTypes = CustomPropertyAppliesTo.Dialogs;
            }
            else if (objectThatHasProperties is GUI)
            {
                propertyTypes = CustomPropertyAppliesTo.GUIs;
            }
            else if (objectThatHasProperties is GUIControl)
            {
                propertyTypes = CustomPropertyAppliesTo.GUIControls;
            }
            else if (objectThatHasProperties is InventoryItem)
            {
                propertyTypes = CustomPropertyAppliesTo.InventoryItems;
            }
            else if (objectThatHasProperties is AudioClip)
            {
                propertyTypes = CustomPropertyAppliesTo.AudioClips;
            }
            else if (objectThatHasProperties is Room)
            {
                propertyTypes = CustomPropertyAppliesTo.Rooms;
            }
            else if (objectThatHasProperties is RoomObject)
            {
                propertyTypes = CustomPropertyAppliesTo.Objects;
            }
            else if (objectThatHasProperties is RoomHotspot)
            {
                propertyTypes = CustomPropertyAppliesTo.Hotspots;
            }
            else if (objectThatHasProperties is RoomRegion)
            {
                propertyTypes = CustomPropertyAppliesTo.Regions;
            }
            else if (objectThatHasProperties is RoomWalkableArea)
            {
                propertyTypes = CustomPropertyAppliesTo.WalkableAreas;
            }
            else
            {
                throw new AGSEditorException("Object not recognised to contain properties: " + objectThatHasProperties.ToString());
            }

            Factory.GUIController.CustomPropertiesEditor(props, propertyTypes);
        }

        private AudioClip ShowAudioClipSourceFileChooserFromPropertyGrid(AudioClip audioClip)
        {
            Factory.ComponentController.FindComponent<AudioComponent>()?.ReplaceAudioClipSource(audioClip);
            return audioClip;
        }

        private int? ShowSpriteChooserFromPropertyGrid(int? currentSprite)
        {
            int defaultSpriteInDialog = currentSprite ?? _lastSelectedSprite;
            Sprite chosenSprite = SpriteChooser.ShowSpriteChooser(defaultSpriteInDialog);
            if (chosenSprite == null)
            {
                return currentSprite;
            }
			_lastSelectedSprite = chosenSprite.Number;
            return chosenSprite.Number;
        }

        private int ShowViewChooserFromPropertyGrid(int currentView)
        {
            AGS.Types.View chosenView = ViewChooser.ShowViewChooser(null, currentView);
            if (chosenView == null)
            {
                return currentView;
            }
            return chosenView.ID;
        }

        public void StartGUI(CommandLineOptions options)
        {
            _commandOptions = options;
            _messageLoopStarted = true;
            Application.Run(_mainForm);
        }

        void _mainForm_OnPropertyObjectChanged(object newPropertyObject)
        {
            if ((ActivePane != null) && (ActivePane.PropertyGridObjectList.ContainsValue(newPropertyObject)))
            {
                ActivePane.SelectedPropertyGridObject = newPropertyObject;
                ActivePane.SelectedPropertyGridObjects = null;
            }

            if (OnPropertyObjectChanged != null)
            {
                OnPropertyObjectChanged(newPropertyObject);
            }
        }

        void _mainForm_OnPropertyChanged(string propertyName, object oldValue)
        {
            if (ActivePane != null)
            {               
                ActivePane.Owner.PropertyChanged(propertyName, oldValue);
                ActivePane.Control.PropertyChanged(propertyName, oldValue);
                DocumentTitlesChanged(); // TODO: only update title when certain properties change?
            }
        }

        void _mainForm_OnMenuClick(string menuItemID)
        {
            string[] results = menuItemID.Split(new string[] { CONTROL_ID_SPLIT }, StringSplitOptions.None);
            _menuItems[menuItemID].CommandClick(results[1]);
        }

		public bool CanFindComponentFromMenuItemID(string menuItemID)
		{
			return menuItemID.Contains(CONTROL_ID_SPLIT);
		}

        private void _mainForm_OnActiveDocumentChanged()
        {
            _toolBarManager.ShowPane(ActivePane);
            _menuManager.ShowPane(ActivePane);
        }

        internal void RegisterContextMenuCommands(IList<MenuCommand> commands, IEditorComponent plugin)
        {
            foreach (MenuCommand command in commands)
            {
                if (command.ID != null)
                {
                    command.ID = RegisterMenuCommand(command.ID, plugin);
                }
            }
        }

        private string GetMenuCommandID(string id, IEditorComponent component)
        {
            if (!id.Contains(CONTROL_ID_SPLIT))
            {
                id = component.ComponentID + CONTROL_ID_SPLIT + id;
            }
            return id;
        }

        private void UnregisterMenuCommand(string id)
        {
            _menuItems.Remove(id);
        }

        private string RegisterMenuCommand(string id, IEditorComponent component)
        {
            id = GetMenuCommandID(id, component);

            if (!_menuItems.ContainsKey(id))
            {
                _menuItems.Add(id, component);
            }

            return id;
        }

		public bool QueryWhetherToSaveGameBeforeContinuing(string message)
		{
            if (StdConsoleWriter.IsEnabled || !_agsEditor.TestIfCanSaveNow())
                return false;

            bool proceed = true;
			DialogResult result = MessageBox.Show(message, "Save changes?", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
			if (result == DialogResult.Cancel)
			{
				proceed = false;
			}
			else
			{
				if ((result == DialogResult.No) && (_agsEditor.CurrentGame.FilesAddedOrRemoved))
				{
					result = MessageBox.Show("Files have been added, removed or renamed within the game. If you don't save the game now, the project may become inconsistent and require manual fixing upon loading it next time. Do you want to save your changes?", "Save changes?", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
				}

				if (result == DialogResult.Yes)
				{
					_agsEditor.SaveGameFiles();
				}

                if ((Factory.AGSEditor.Settings.BackupWarningInterval > 0) &&
                    (DateTime.Now.Subtract(Factory.AGSEditor.Settings.LastBackupWarning).TotalDays > Factory.AGSEditor.Settings.BackupWarningInterval))
                {
                    Factory.AGSEditor.Settings.LastBackupWarning = DateTime.Now;
                    Factory.AGSEditor.Settings.Save();
                    this.ShowMessage("Have you backed up your game recently? Remember, power failures and blue screens happen when you least expect them. Make a backup copy of your game now!", MessageBoxIcon.Warning);
                }
			}
			return proceed;
		}

        /// <summary>
        /// Tests if the Editor can close the project and shutdown application.
        /// Is allowed to show error messages in the process.
        /// Returns the result.
        /// FIXME: these methods and events are disorganized:
        /// - QueryEditorShutdown is in GUIController,
        /// - AttemptToSaveGame is in AGSEditor,
        /// need to bring this to some consistency.
        /// </summary>
        public bool TestIfCanShutdownNow()
        {
            if (QueryEditorShutdown != null)
            {
                return QueryEditorShutdown();
            }
            return true;
        }

        private bool _mainForm_OnEditorShutdown()
        {
            SaveEditorWindowSize();

            bool canShutDown = true;

			if (_batchProcessShutdown || _exitFromWelcomeScreen)
			{
                OnEditorShutdown?.Invoke();
            }
			else
			{
                canShutDown = TestIfCanShutdownNow();
                if (canShutDown)
                {
                    canShutDown = QueryWhetherToSaveGameBeforeContinuing("Do you want to save the game before exiting?");
                }
				if (canShutDown)
				{
                    OnEditorShutdown?.Invoke();
                }
			}

			if (canShutDown)
			{
				SystemEvents.DisplaySettingsChanged -= new EventHandler(SystemEvents_DisplaySettingsChanging);
                _agsEditor.Dispose();
			}

            return canShutDown;
        }

        Sprite IGUIController.ShowSpriteSelector(int initiallySelectedSprite)
        {
            return SpriteChooser.ShowSpriteChooser(initiallySelectedSprite);
        }

        public void UpdateStatusBarText(string text)
        {
            _mainForm.statusLabel.Text = text;
        }

        public void RePopulateTreeView(IEditorComponent component, string selectedNode)
        {
            IRePopulatableComponent repopulatableComponent = component as IRePopulatableComponent;
            if (repopulatableComponent != null)
            {
                if (selectedNode == null)
                {
                    repopulatableComponent.RePopulateTreeView();
                }
                else
                {
                    repopulatableComponent.RePopulateTreeView(selectedNode);
                }
            }
        }

        public void RePopulateTreeView(IEditorComponent component)
        {
            RePopulateTreeView(component, null);
        }
        public bool ShowTabIcons
        {
            get { return _mainForm.ShowTabIcons; }
            set { _mainForm.ShowTabIcons = value; }
        }


        void IGUIController.SetStatusBarText(string text)
        {
            if (_mainForm.InvokeRequired)
            {
                _mainForm.Invoke(new UpdateStatusBarTextDelegate(UpdateStatusBarText), text);
            }
            else
            {
                UpdateStatusBarText(text);
            }
        }

        private void AutoComplete_BackgroundCacheUpdateStatusChanged(BackgroundAutoCompleteStatus status, Exception errorDetails)
        {
            string statusText = string.Empty;
            switch (status) 
            {
                case BackgroundAutoCompleteStatus.Processing:
                    statusText = "Updating autocomplete cache...";
                    break;
                case BackgroundAutoCompleteStatus.Finished:
                    statusText = string.Empty;
                    break;
                case BackgroundAutoCompleteStatus.Error:
                    statusText = "AutoComplete error: " + errorDetails.Message;
                    break;
            }
            ((IGUIController)this).SetStatusBarText(statusText);
        }

        private void SetEditorWindowSize()
        {
            _mainForm.SetDesktopLocation(Factory.AGSEditor.Settings.MainWinX, Factory.AGSEditor.Settings.MainWinY);
            _mainForm.Width = Math.Max(Factory.AGSEditor.Settings.MainWinWidth, 300);
            _mainForm.Height = Math.Max(Factory.AGSEditor.Settings.MainWinHeight, 300);
            _mainForm.WindowState = Factory.AGSEditor.Settings.MainWinMaximize ? FormWindowState.Maximized : FormWindowState.Normal;
        }

        private void SaveEditorWindowSize()
        {
            if (_mainForm.WindowState != FormWindowState.Minimized)
            {
                Factory.AGSEditor.Settings.MainWinMaximize =_mainForm.WindowState == FormWindowState.Maximized ? true : false;

                if (_mainForm.WindowState == FormWindowState.Normal)
                {
                    Factory.AGSEditor.Settings.MainWinWidth = _mainForm.Width;
                    Factory.AGSEditor.Settings.MainWinHeight = _mainForm.Height;;
                    Factory.AGSEditor.Settings.MainWinX = _mainForm.Left;
                    Factory.AGSEditor.Settings.MainWinY = _mainForm.Top;
                }

                Factory.AGSEditor.Settings.Save();
            }
        }

        void IGUIController.DrawSprite(Graphics g, int spriteNumber, int x, int y, int width, int height, bool centreHorizontally)
		{
            SpriteInfo info = Factory.NativeProxy.GetSpriteInfo(spriteNumber);

            int spriteWidth = info.Width;
			int spriteHeight = info.Height;
			Bitmap bmp = Utilities.GetBitmapForSpriteResizedKeepingAspectRatio(new Sprite(spriteNumber, spriteWidth, spriteHeight), width, height, centreHorizontally, false, SystemColors.Control);
			g.DrawImage(bmp, x, y);
			bmp.Dispose();
		}

        private struct AutoNumberSpeechData
        {
            internal AutoNumberSpeechOptions Options;
            internal int? CharacterID;
        }

        private enum AutoNumberSpeechOptions
        {
            None = 0,
            DoNarrator = 1,
            CombineIdenticalLines = 2,
            RemoveNumbering = 4
        }
    }
}
