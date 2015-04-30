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
using AGS.Types.AutoComplete;
using AGS.Types.Interfaces;

namespace AGS.Editor
{
    public class GUIController : IGUIController
    {
        public const string IMAGE_FILE_FILTER = "All supported images (*.bmp; *.gif; *.jpg; *.png; *.tif)|*.bmp;*.gif;*.jpg;*.png;*.tif|Windows bitmap files (*.bmp)|*.bmp|Compuserve Graphics Interchange (*.gif)|*.gif|JPEG (*.jpg)|*.jpg|Portable Network Graphics (*.png)|*.png|Tagged Image File (*.tif)|*.tif";

        public const string FILE_MENU_ID = "fileToolStripMenuItem";
        public const string HELP_MENU_ID = "HelpMenu";
        private const string CONTROL_ID_SPLIT = "^!^";
        private const string TEMPLATE_INTRO_FILE = "template.txt";
		private const string ROOM_TEMPLATE_ID_FILE = "rtemplate.dat";
		private const int ROOM_TEMPLATE_ID_FILE_SIGNATURE = 0x74673812;

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
        public delegate void GetScriptEditorControlHandler(GetScriptEditorControlEventArgs evArgs);
        public event GetScriptEditorControlHandler OnGetScriptEditorControl;
        public delegate void ScriptChangedHandler(Script script);
        public event ScriptChangedHandler OnScriptChanged;
        public delegate void LaunchHelpHandler(string keyword);
        public event LaunchHelpHandler OnLaunchHelp;
        public event EventHandler OnMainWindowActivated;

        private delegate void UpdateStatusBarTextDelegate(string text);
        private delegate void ParameterlessDelegate();
        private delegate void ZoomToFileDelegate(string fileName, ZoomToFileZoomType zoomType, int lineNumber, bool isDebugExecutionPoint, bool selectWholeLine, string errorMessage, bool activateEditor);
        private delegate void ShowCallStackDelegate(DebugCallStack callStack);
        private delegate void ShowFindSymbolResultsDelegate(List<ScriptTokenReference> results);

        private frmMain _mainForm;
        private Dictionary<string, IEditorComponent> _menuItems;
        private ImageList _imageList = new ImageList();
        private ProjectTree _treeManager;
        private ToolBarManager _toolBarManager;
        private MainMenuManager _menuManager;
        private string _titleBarPrefix;
        private AGSEditor _agsEditor;
        private InteractiveTasks _interactiveTasks;
        private bool _exitFromWelcomeScreen = false;
		private bool _batchProcessShutdown = false;
		private int _lastSelectedSprite = 0;
		private string _lastImportDirectory = null;
		private string[] _commandLineArgs;
		private bool _messageLoopStarted = false;
        private int _systemDpi = 0;
		//private HelpPopup _cuppit;

        private string _timerScriptName;
        private string _timerSearchForText;

        private GUIController()
        {
            _menuItems = new Dictionary<string, IEditorComponent>();
            this.InitializeColorTheme();
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

        public ColorTheme UserColorTheme
        {
            get;
            private set;
        }

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

        public void AddMenu(IEditorComponent plugin, string id, string title)
        {
            _menuManager.AddMenu(id, title);
        }

		public void AddMenu(IEditorComponent plugin, string id, string title, string insertAfterMenu)
		{
			_menuManager.AddMenu(id, title, insertAfterMenu);
		}

		public void AddMenuItems(IEditorComponent plugin, MenuCommands commands)
        {
            if (commands.Commands.Count > 0)
            {
                foreach (MenuCommand command in commands.Commands)
                {
                    if (command.ID != null)
                    {
                        RegisterMenuCommand(command.ID, plugin);
                    }
                    command.IDPrefix = plugin.ComponentID + CONTROL_ID_SPLIT;
                }
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

        public void AddOrShowPane(ContentDocument pane)
        {
            _mainForm.AddOrShowPane(pane);
        }

        public void RemovePaneIfExists(ContentDocument pane)
        {
            _mainForm.RemovePaneIfExists(pane);
        }

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
                _mainForm.pnlOutput.Show();
            }
        }

        public void ClearOutputPanel()
        {
            _mainForm.pnlOutput.ErrorsToList = null;
        }

        public void HideOutputPanel()
        {
            if (_mainForm.pnlOutput.InvokeRequired)
            {
                _mainForm.pnlOutput.Invoke(new ParameterlessDelegate(HideOutputPanel));
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

        public void HideFindSymbolResults()
        {
            _mainForm.pnlFindResults.Hide();
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
					ZoomToFileEventArgs evArgs = new ZoomToFileEventArgs(fileName, zoomType, zoomPosition, null, isDebugExecutionPoint, errorMessage, activateEditor);
					evArgs.SelectLine = selectWholeLine;
					OnZoomToFile(evArgs);
                }
            }
        }

        public void ZoomToFile(string fileName, string function)
        {
            if (OnZoomToFile != null)
            {
				ZoomToFileEventArgs evArgs = new ZoomToFileEventArgs(fileName, ZoomToFileZoomType.ZoomToText, 0, "function " + function + "(", false, null, true);
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

        public object GetPropertyGridObject()
        {
            if (_mainForm.ActivePane == null) return null;
            return _mainForm.ActivePane.SelectedPropertyGridObject;
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

        public string ShowSaveFileDialog(string title, string fileFilter)
        {
            SaveFileDialog dialog = new SaveFileDialog();
            dialog.Title = title;
            dialog.RestoreDirectory = true;
            dialog.OverwritePrompt = true;
            dialog.ValidateNames = true;
            dialog.InitialDirectory = System.IO.Directory.GetCurrentDirectory();
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
					(_agsEditor.Preferences.DefaultImportPath.Length > 0))
				{
					_lastImportDirectory = _agsEditor.Preferences.DefaultImportPath;
				}
				else
				{
					_lastImportDirectory = System.IO.Directory.GetCurrentDirectory();
				}
			}
		}

        public string ShowOpenFileDialog(string title, string fileFilter, bool useFileImportPath)
        {
			EnsureLastImportDirectoryIsSet(useFileImportPath);

			OpenFileDialog dialog = new OpenFileDialog();
			dialog.Title = title;
			dialog.RestoreDirectory = true;
			dialog.CheckFileExists = true;
			dialog.CheckPathExists = true;
			dialog.InitialDirectory = _lastImportDirectory;
			dialog.ValidateNames = true;
            dialog.Filter = fileFilter;

            if (dialog.ShowDialog() == DialogResult.OK)
            {
				_lastImportDirectory = Path.GetDirectoryName(dialog.FileName);
                return dialog.FileName;
            }
            return null;
        }

        public string[] ShowOpenFileDialogMultipleFiles(string title, string fileFilter)
        {
			EnsureLastImportDirectoryIsSet(true);

            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Title = title;
            dialog.RestoreDirectory = true;
            dialog.CheckFileExists = true;
            dialog.CheckPathExists = true;
            dialog.InitialDirectory = _lastImportDirectory;
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

        public bool ShowCheckOutDialog(string fileName)
        {
            List<string> fileNames = new List<string>();
            fileNames.Add(fileName);
            return ShowCheckOutDialog(fileNames);
        }

        public bool ShowCheckOutDialog(List<string> fileNames)
        {
            bool checkedOut = false;
            CheckinsDialog dialog = new CheckinsDialog("Check Out", "Check out", fileNames.ToArray());
            dialog.ShowDialog();
            if (dialog.SelectedFiles != null)
            {
				try
				{
					_agsEditor.SourceControlProvider.CheckOutFiles(dialog.SelectedFiles, dialog.Comments);
					checkedOut = (dialog.SelectedFiles.Length == fileNames.Count);
				}
				catch (SourceControlException ex)
				{
					this.ShowMessage("Checkout failed: " + ex.SccErrorMessage, MessageBoxIcon.Warning);
					checkedOut = false;
				}
            }
            dialog.Dispose();
            return checkedOut;
        }

        public void ShowPendingCheckinsDialog()
        {
            List<string> fileNamesToShow = new List<string>();
			string[] files = _agsEditor.GetFilesThatCanBePutUnderSourceControl();
            SourceControlFileStatus[] fileStatuses = _agsEditor.SourceControlProvider.GetFileStatuses(files);
            for (int i = 0; i < files.Length; i++)
            {
                if (((fileStatuses[i] == SourceControlFileStatus.NotControlled) ||
                    ((fileStatuses[i] & SourceControlFileStatus.Deleted) != 0) ||
                    ((fileStatuses[i] & SourceControlFileStatus.CheckedOutByMe) != 0)) &&
                    (fileStatuses[i] != SourceControlFileStatus.Invalid))
                {
                    fileNamesToShow.Add(files[i]);
                }
            }

            CheckinsDialog dialog = new CheckinsDialog("Pending Checkins", "Check in", fileNamesToShow.ToArray());
            dialog.ShowDialog();
            string[] selectedFiles = dialog.SelectedFiles;
            string checkinComments = dialog.Comments;
            dialog.Dispose();

            if (selectedFiles != null)
            {
                CheckInOrAddFiles(selectedFiles, checkinComments);
            }

        }

        private void CheckInOrAddFiles(string[] selectedFiles, string checkinComments)
        {
            List<string> filesToCheckin = new List<string>();
            List<string> filesToAdd = new List<string>();

            foreach (string fileName in selectedFiles)
            {
                SourceControlFileStatus[] fileStatus = _agsEditor.SourceControlProvider.GetFileStatuses(new string[] { fileName });
                if ((fileStatus[0] == SourceControlFileStatus.NotControlled) ||
                    ((fileStatus[0] & SourceControlFileStatus.Deleted) != 0))
                {
                    filesToAdd.Add(fileName);
                }
                else
                {
                    filesToCheckin.Add(fileName);
                }
            }

			try
			{
				if (filesToAdd.Count > 0)
				{
					_agsEditor.SourceControlProvider.AddFilesToSourceControl(filesToAdd.ToArray(), checkinComments);
				}
				if (filesToCheckin.Count > 0)
				{
					_agsEditor.SourceControlProvider.CheckInFiles(filesToCheckin.ToArray(), checkinComments);
				}
			}
			catch (SourceControlException ex)
			{
				this.ShowMessage("Check-in failed: " + ex.SccErrorMessage, MessageBoxIcon.Warning);
			}
        }

        public void Initialize(AGSEditor agsEditor)
        {
            if (_mainForm == null)
            {
                _agsEditor = agsEditor;
                _interactiveTasks = new InteractiveTasks(_agsEditor.Tasks);
                _mainForm = new frmMain();
                SetEditorWindowSizeFromRegistry();
                _treeManager = new ProjectTree(_mainForm.projectPanel.projectTree);
                _treeManager.OnContextMenuClick += new ProjectTree.MenuClickHandler(_mainForm_OnMenuClick);
                _toolBarManager = new ToolBarManager(_mainForm.toolStrip);
                WindowsMenuManager windowsMenuManager = new WindowsMenuManager(_mainForm.windowsToolStripMenuItem, 
                    _mainForm.GetStartupPanes(), _mainForm.mainContainer);
                _menuManager = new MainMenuManager(_mainForm.mainMenu, windowsMenuManager);
                _mainForm.OnEditorShutdown += new frmMain.EditorShutdownHandler(_mainForm_OnEditorShutdown);
                _mainForm.OnPropertyChanged += new frmMain.PropertyChangedHandler(_mainForm_OnPropertyChanged);
                _mainForm.OnPropertyObjectChanged += new frmMain.PropertyObjectChangedHandler(_mainForm_OnPropertyObjectChanged);
                _mainForm.OnActiveDocumentChanged += new frmMain.ActiveDocumentChangedHandler(_mainForm_OnActiveDocumentChanged);
                _mainForm.OnMainWindowActivated += new EventHandler(_mainForm_OnMainWindowActivated);
                _menuManager.OnMenuClick += new MainMenuManager.MenuClickHandler(_mainForm_OnMenuClick);
                AutoComplete.BackgroundCacheUpdateStatusChanged += new AutoComplete.BackgroundCacheUpdateStatusChangedHandler(AutoComplete_BackgroundCacheUpdateStatusChanged);
				SystemEvents.DisplaySettingsChanged += new EventHandler(SystemEvents_DisplaySettingsChanging);

                RegisterIcon("GameIcon", Resources.ResourceManager.GetIcon("game.ico"));
				RegisterIcon("CompileErrorIcon", Resources.ResourceManager.GetIcon("eventlogError.ico"));
				RegisterIcon("CompileWarningIcon", Resources.ResourceManager.GetIcon("eventlogWarn.ico"));
				_mainForm.SetTreeImageList(_imageList);
                _mainForm.mainMenu.ImageList = _imageList;
				_mainForm.pnlOutput.SetImageList(_imageList);
				//_mainForm.SetProjectTreeLocation(_agsEditor.Preferences.ProjectTreeOnRight);

                ViewUIEditor.ViewSelectionGUI = new ViewUIEditor.ViewSelectionGUIType(ShowViewChooserFromPropertyGrid);
                SpriteSelectUIEditor.SpriteSelectionGUI = new SpriteSelectUIEditor.SpriteSelectionGUIType(ShowSpriteChooserFromPropertyGrid);
                CustomPropertiesUIEditor.CustomPropertiesGUI = new CustomPropertiesUIEditor.CustomPropertiesGUIType(ShowPropertiesEditorFromPropertyGrid);
                PropertyTabInteractions.UpdateEventName = new PropertyTabInteractions.UpdateEventNameHandler(PropertyTabInteractions_UpdateEventName);
                ScriptFunctionUIEditor.OpenScriptEditor = new ScriptFunctionUIEditor.OpenScriptEditorHandler(ScriptFunctionUIEditor_OpenScriptEditor);
                ScriptFunctionUIEditor.CreateScriptFunction = new ScriptFunctionUIEditor.CreateScriptFunctionHandler(ScriptFunctionUIEditor_CreateScriptFunction);
                RoomMessagesUIEditor.ShowRoomMessagesEditor = new RoomMessagesUIEditor.RoomMessagesEditorType(ShowRoomMessageEditorFromPropertyGrid);
                CustomResolutionUIEditor.CustomResolutionSetGUI = new CustomResolutionUIEditor.CustomResolutionGUIType(ShowCustomResolutionChooserFromPropertyGrid);
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

        public void ExitApplication()
        {
            _mainForm.Close();
        }

		public void ShowCuppit(string helpText, string helpTextID)
		{
			ShowCuppit(helpText, helpTextID, false);
		}

		public void ShowCuppit(string helpText, string helpTextID, bool modal)
		{
/*          Disabled for now, Cuppit was not too popular!!
            if (_cuppit == null)
			{
				_cuppit = new HelpPopup();
			}

			if (_cuppit.ShouldShow(helpTextID, modal))
			{
				_cuppit.Location = new Point(_mainForm.Left + 20, _mainForm.Bottom - 300);
				_cuppit.SetHelpText(helpText);

				if (modal)
				{
					_cuppit.ShowDialog(_mainForm);
				}
				else
				{
					_cuppit.Show(_mainForm);
				}
			}*/
		}

		private bool ProcessCommandLineArgumentsAndReturnWhetherToShowWelcomeScreen()
		{
			bool compileAndExit = false;
			bool forceRebuild;

			foreach (string arg in _commandLineArgs)
			{
				if (arg.ToLower() == "/compile")
				{
					compileAndExit = true;
				}
				else if (arg.StartsWith("/") || arg.StartsWith("-"))
				{
					this.ShowMessage("Invalid command line argument " + arg, MessageBoxIcon.Warning);
				}
				else
				{
					if (!File.Exists(arg))
					{
						this.ShowMessage("Unable to load the game '" + arg + "' because it does not exist", MessageBoxIcon.Warning);
					}
					else if (_interactiveTasks.LoadGameFromDisk(arg))
					{
						if (compileAndExit)
						{
							forceRebuild = _agsEditor.NeedsRebuildForDebugMode();
							if (forceRebuild)
								_agsEditor.SaveGameFiles();
							if (!_agsEditor.CompileGame(forceRebuild, false).HasErrors)
							{
								_batchProcessShutdown = true;
								this.ExitApplication();
							}
						}
						return false;
					}
				}
			}
			return true;
		}

        public bool VerifyTemplatesDirectoryExists()
        {
            if (!Directory.Exists(_agsEditor.TemplatesDirectory))
            {
                try
                {
                    Directory.CreateDirectory(_agsEditor.TemplatesDirectory);
                }
                catch (UnauthorizedAccessException ex)
                {
                    this.ShowMessage("The Templates directory was missing and could not be created. AGS may not be installed properly or you may not have the appropriate permissions to run it." + Environment.NewLine + ex.Message, MessageBoxIcon.Error);
                    return false;
                }
            }
            return true;
        }

        public bool ShowWelcomeScreen()
        {
            if (System.Environment.OSVersion.Platform != PlatformID.Win32NT)
			{
				this.ShowMessage("You are running AGS on a computer with Windows 98 or Windows ME. AGS is no longer supported on these operating systems. You are STRONGLY ADVISED to run the AGS Editor on Windows 2000, XP or Vista.", MessageBoxIcon.Warning);
			}

            if (!VerifyTemplatesDirectoryExists())
            {
                _exitFromWelcomeScreen = true;
                Application.Exit();
                return true;
            }

			bool showWelcomeScreen = ProcessCommandLineArgumentsAndReturnWhetherToShowWelcomeScreen();

            while (showWelcomeScreen)
            {
				Directory.SetCurrentDirectory(_agsEditor.EditorDirectory);
                WelcomeScreen welcomeScreen = new WelcomeScreen(_agsEditor.RecentGames);
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
                    string gameToLoad = Path.Combine(welcomeScreen.SelectedRecentGame.DirectoryPath, AGSEditor.GAME_FILE_NAME);
                    if (!File.Exists(gameToLoad))
                    {
                        gameToLoad = gameToLoad.Replace(AGSEditor.GAME_FILE_NAME, AGSEditor.OLD_GAME_FILE_NAME);
                        if (!File.Exists(gameToLoad))
                        {
                            Factory.GUIController.ShowMessage("Unable to find a valid game file in " + welcomeScreen.SelectedRecentGame.DirectoryPath, MessageBoxIcon.Warning);
                            showWelcomeScreen = true;
                        }
                        else if (!_interactiveTasks.LoadGameFromDisk(gameToLoad))
                        {
                            showWelcomeScreen = true;
                        }
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
            foreach (string fileName in Utilities.GetDirectoryFileList(_agsEditor.TemplatesDirectory, "*.agt"))
            {
                GameTemplate template = Factory.NativeProxy.LoadTemplateFile(fileName);
                if (template != null)
                {
                    templates.Add(template);
                }
            }

            List<WizardPage> pages = new List<WizardPage>();
            StartNewGameWizardPage templateSelectPage = new StartNewGameWizardPage(templates);
            StartNewGameWizardPage2 gameNameSelectPage = new StartNewGameWizardPage2(_agsEditor.Preferences.NewGamePath);
            pages.Add(templateSelectPage);
            pages.Add(gameNameSelectPage);
            
            WizardDialog dialog = new WizardDialog("Start New Game", "This wizard will guide you through the process of creating a new game.", pages);
            DialogResult result = dialog.ShowDialog();

            if (result == DialogResult.OK)
            {
                createdSuccessfully = CreateNewGame(gameNameSelectPage.GetFullPath(), gameNameSelectPage.NewGameName, templateSelectPage.SelectedTemplate);
            }

            dialog.Dispose();
            return createdSuccessfully;
        }

        private bool CreateNewGame(string newGameDirectory, string newGameName, GameTemplate createFromTemplate)
        {
            bool createdSuccessfully = false;
            try
            {
                string templateFileName = Path.Combine(_agsEditor.EditorDirectory, createFromTemplate.FileName);
                _agsEditor.Tasks.CreateNewGameFromTemplate(templateFileName, newGameDirectory);
                string newGameFileName = Path.Combine(newGameDirectory, AGSEditor.GAME_FILE_NAME);
                if (!File.Exists(newGameFileName))
                {
                    newGameFileName = Path.Combine(newGameDirectory, AGSEditor.OLD_GAME_FILE_NAME);
                }
                if (_agsEditor.Tasks.LoadGameFromDisk(newGameFileName, false))
                {
                    _agsEditor.CurrentGame.Settings.GameName = newGameName;
                    _agsEditor.CurrentGame.Settings.SaveGameFolderName = newGameName;
                    _agsEditor.CurrentGame.Settings.GenerateNewGameID();
                    Factory.GUIController.GameNameUpdated();
                    _agsEditor.CurrentGame.WorkspaceState.LastBuildConfiguration = _agsEditor.CurrentGame.Settings.DebugMode ? BuildConfiguration.Debug : BuildConfiguration.Release;
                    if (_agsEditor.SaveGameFiles())
                    {
                        // Force a rebuild to remove the key in the room
                        // files that links them to the old game ID
                        // Force no message to be displayed if the build fails
                        // (which it will for the Empty Game template)
                        MessageBoxOnCompile oldMessageBoxSetting = _agsEditor.Preferences.MessageBoxOnCompileErrors;
                        _agsEditor.Preferences.MessageBoxOnCompileErrors = MessageBoxOnCompile.Never;

                        _agsEditor.CompileGame(true, false);

                        _agsEditor.Preferences.MessageBoxOnCompileErrors = oldMessageBoxSetting;
                    }
                    if (File.Exists(TEMPLATE_INTRO_FILE))
                    {
                        StreamReader sr = new StreamReader(TEMPLATE_INTRO_FILE);
                        string introText = sr.ReadToEnd();
                        sr.Close();

                        Factory.GUIController.ShowMessage(introText, MessageBoxIcon.Information);
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
                if (Directory.Exists(newGameDirectory))
                {
                    Directory.Delete(newGameDirectory, true);
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

		public void ShowCreateRoomTemplateWizard(UnloadedRoom room)
		{
			List<WizardPage> pages = new List<WizardPage>();
			MakeTemplateWizardPage templateCreationPage = new MakeTemplateWizardPage(_agsEditor.TemplatesDirectory, ".art");
			pages.Add(templateCreationPage);

			WizardDialog dialog = new WizardDialog("Room Template", "This wizard will guide you through the process of creating a room template. A room template allows you to easily create new rooms based off an existing one.", pages);
			DialogResult result = dialog.ShowDialog();

			if (result == DialogResult.OK)
			{
				string templateFileName = templateCreationPage.GetFullPath();
				try
				{
					if (File.Exists(templateFileName))
					{
						File.Delete(templateFileName);
					}
					BinaryWriter writer = new BinaryWriter(new FileStream(ROOM_TEMPLATE_ID_FILE, FileMode.Create, FileAccess.Write));
					writer.Write(ROOM_TEMPLATE_ID_FILE_SIGNATURE);
					writer.Write(room.Number);
					writer.Close();

					Factory.NativeProxy.CreateTemplateFile(templateFileName, ConstructRoomTemplateFileList(room));

					File.Delete(ROOM_TEMPLATE_ID_FILE);

					Factory.GUIController.ShowMessage("Template created successfully. To try it out, create a new room.", MessageBoxIcon.Information);
				}
				catch (AGSEditorException ex)
				{
					Factory.GUIController.ShowMessage("There was an error creating your template: " + ex.Message, MessageBoxIcon.Warning);
				}
				catch (Exception ex)
				{
					Factory.GUIController.ShowMessage("There was an error creating your template. The error was: " + ex.Message + Environment.NewLine + Environment.NewLine + "Error details: " + ex.ToString(), MessageBoxIcon.Warning);
				}
			}

			dialog.Dispose();
		}

        public void ShowCreateTemplateWizard()
        {
            if (_agsEditor.SourceControlProvider.ProjectUnderControl)
            {
                if (this.ShowQuestion("This game is under source control. It is not advisable to create a template that is bound to source control. Are you sure you want to continue?", MessageBoxIcon.Warning) == DialogResult.No)
                {
                    return;
                }
            }

            List<WizardPage> pages = new List<WizardPage>();
            MakeTemplateWizardPage templateCreationPage = new MakeTemplateWizardPage(_agsEditor.TemplatesDirectory, ".agt");
            pages.Add(templateCreationPage);

            WizardDialog dialog = new WizardDialog("Create Template", "This wizard will guide you through the process of creating a template. A template allows other people to easily create a game that uses your game as a starting point.", pages);
            DialogResult result = dialog.ShowDialog();

            if (result == DialogResult.OK)
            {
                string templateName = templateCreationPage.GetFullPath();
                try
                {
                    _agsEditor.SaveGameFiles();
                    InteractiveTasks.CreateTemplateFromCurrentGame(templateName);
                    Factory.GUIController.ShowMessage("Template created successfully. To try it out, close AGS, start it up again, and select the 'Start New Game' option.", MessageBoxIcon.Information);
                }
				catch (AGSEditorException ex)
				{
					Factory.GUIController.ShowMessage("There was an error creating your template:" + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
				}
				catch (Exception ex)
                {
                    Factory.GUIController.ShowMessage("There was an error creating your template. The error was: " + ex.Message + Environment.NewLine + Environment.NewLine + "Error details: " + ex.ToString(), MessageBoxIcon.Warning);
                }
            }

            dialog.Dispose();
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

        private object AutoNumberSpeechLinesProcess(object parameter)
        {
            AutoNumberSpeechData data = (AutoNumberSpeechData)parameter;
            bool doNarrator = (data.Options & AutoNumberSpeechOptions.DoNarrator) != 0;
            bool combineIdenticalLines = (data.Options & AutoNumberSpeechOptions.CombineIdenticalLines) != 0;
            bool removeNumbering = (data.Options & AutoNumberSpeechOptions.RemoveNumbering) != 0;
            return new SpeechLinesNumbering().NumberSpeechLines(_agsEditor.CurrentGame, doNarrator, combineIdenticalLines, removeNumbering, data.CharacterID);
        }

		private object CreateVoiceActingScriptProcess(object parameter)
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
            PreferencesEditor prefsEditor = new PreferencesEditor(_agsEditor.Preferences);
            if (prefsEditor.ShowDialog() == DialogResult.OK)
            {
                _agsEditor.Preferences.SaveToRegistry();
				//_mainForm.SetProjectTreeLocation(_agsEditor.Preferences.ProjectTreeOnRight);
            }
            prefsEditor.Dispose();
        }

        private void ScriptFunctionUIEditor_CreateScriptFunction(bool isGlobalScript, string functionName, string parameters)
        {
            string scriptToRetrieve = Script.GLOBAL_SCRIPT_FILE_NAME;
            if (!isGlobalScript)
            {
                scriptToRetrieve = Script.CURRENT_ROOM_SCRIPT_FILE_NAME;
            }

            if (OnGetScript != null)
            {
                Script script = null;
                OnGetScript(scriptToRetrieve, ref script);
                if (script != null)
                {
                    string functionStart = "function " + functionName + "(";
                    if (script.Text.IndexOf(functionStart) < 0)
                    {
                        if (_agsEditor.AttemptToGetWriteAccess(script.FileName))
                        {
                            script.Text += Environment.NewLine + functionStart + parameters + ")" + Environment.NewLine;
                            script.Text += "{" + Environment.NewLine + Environment.NewLine + "}" + Environment.NewLine;
                            if (OnScriptChanged != null)
                            {
                                OnScriptChanged(script);
                            }
                        }
                    }
                }
            }
        }

        private void ScriptFunctionUIEditor_OpenScriptEditor(bool isGlobalScript, string functionName)
        {
            if (OnZoomToFile != null)
            {
                // We need to start a timer, because we are within the
                // property grid processing at the moment
                _timerScriptName = (isGlobalScript) ? Script.GLOBAL_SCRIPT_FILE_NAME : Script.CURRENT_ROOM_SCRIPT_FILE_NAME;
                _timerSearchForText = "function " + functionName + "(";
                Timer timer = new Timer();
                timer.Interval = 100;
                timer.Tick += new EventHandler(timer_Tick);
                timer.Start();
            }
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            ((Timer)sender).Stop();
            ((Timer)sender).Dispose();
            ZoomToFileEventArgs evArgs = new ZoomToFileEventArgs(_timerScriptName, ZoomToFileZoomType.ZoomToText, 0, _timerSearchForText, false, null, true);
			evArgs.SelectLine = false;
            evArgs.ZoomToLineAfterOpeningBrace = true;
			OnZoomToFile(evArgs);
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

        private void ShowPropertiesEditorFromPropertyGrid(CustomProperties props, object objectThatHasProperties)
        {
            CustomPropertyAppliesTo propertyTypes;
            if (objectThatHasProperties is Character)
            {
                propertyTypes = CustomPropertyAppliesTo.Characters;
            }
            else if (objectThatHasProperties is RoomHotspot)
            {
                propertyTypes = CustomPropertyAppliesTo.Hotspots;
            }
            else if (objectThatHasProperties is InventoryItem)
            {
                propertyTypes = CustomPropertyAppliesTo.InventoryItems;
            }
            else if (objectThatHasProperties is RoomObject)
            {
                propertyTypes = CustomPropertyAppliesTo.Objects;
            }
            else if (objectThatHasProperties is Room)
            {
                propertyTypes = CustomPropertyAppliesTo.Rooms;
            }
            else
            {
                throw new AGSEditorException("Object not recognised to contain properties: " + objectThatHasProperties.ToString());
            }

            Factory.GUIController.CustomPropertiesEditor(props, propertyTypes);
        }

        private void ShowRoomMessageEditorFromPropertyGrid(List<RoomMessage> messages)
        {
            RoomMessagesEditor.ShowEditor(messages);
        }

        private int ShowSpriteChooserFromPropertyGrid(int currentSprite)
        {
			int defaultSpriteInDialog = currentSprite;
			if (defaultSpriteInDialog == 0)
			{
				defaultSpriteInDialog = _lastSelectedSprite;
			}
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

        public void StartGUI(string[] commandLineArguments)
        {
			_commandLineArgs = commandLineArguments;
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
                DocumentTitlesChanged();
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
					result = MessageBox.Show("Files have been added, removed or renamed. If you don't save the game now, you may not be able to successfully open this game next time. Do you want to save your changes?", "Save changes?", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
				}

				if (result == DialogResult.Yes)
				{
					_agsEditor.SaveGameFiles();
				}

                if ((_agsEditor.Preferences.BackupWarningInterval > 0) &&
                    (DateTime.Now.Subtract(_agsEditor.Preferences.LastBackupWarning).TotalDays > _agsEditor.Preferences.BackupWarningInterval))
                {
                    _agsEditor.Preferences.LastBackupWarning = DateTime.Now;
                    _agsEditor.Preferences.SaveToRegistry();
                    this.ShowMessage("Have you backed up your game recently? Remember, power failures and blue screens happen when you least expect them. Make a backup copy of your game now!", MessageBoxIcon.Warning);
                }
			}
			return proceed;
		}

        private bool _mainForm_OnEditorShutdown()
        {
            SaveEditorWindowSizeToRegistry();

            bool canShutDown = true;

			if (_batchProcessShutdown)
			{
				if (OnEditorShutdown != null)
				{
					OnEditorShutdown();
				}
			}
			else if (!_exitFromWelcomeScreen)
			{
				canShutDown = QueryWhetherToSaveGameBeforeContinuing("Do you want to save the game before exiting?");
				if (canShutDown)
				{
					if (QueryEditorShutdown != null)
					{
						canShutDown = QueryEditorShutdown();
					}
					if ((canShutDown) && (OnEditorShutdown != null))
					{
						OnEditorShutdown();
					}
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

        private void SetEditorWindowSizeFromRegistry()
        {
            RegistryKey key = Registry.CurrentUser.OpenSubKey(AGSEditor.AGS_REGISTRY_KEY);
            if (key != null)
            {
                try
                {
                    int x = Convert.ToInt32(key.GetValue("MainWinX", _mainForm.Left));
                    int y = Convert.ToInt32(key.GetValue("MainWinY", _mainForm.Top));
                    _mainForm.SetDesktopLocation(x, y);
                    int formWidth = Convert.ToInt32(key.GetValue("MainWinWidth", _mainForm.Width));
                    int formHeight = Convert.ToInt32(key.GetValue("MainWinHeight", _mainForm.Height));
                    _mainForm.Width = Math.Max(formWidth, 300);
                    _mainForm.Height = Math.Max(formHeight, 300);
                    _mainForm.WindowState = (key.GetValue("MainWinMaximize", "0").ToString() == "1") ? FormWindowState.Maximized : FormWindowState.Normal;
                    /*int splitterX = Convert.ToInt32(key.GetValue("MainWinSplitter1", 0));
                    int splitterY = Convert.ToInt32(key.GetValue("MainWinSplitter2", 0));
                    if ((splitterX > 0) && (splitterY > 0))
                    {
                        _mainForm.SetSplitterPositions(splitterX, splitterY);
                    }*/
                }
                catch (Exception ex)
                {
                    this.ShowMessage("There was an error reading your settings from the registry:" + Environment.NewLine + ex.Message + Environment.NewLine + Environment.NewLine +
                        "The AGS registry entries may have been corrupted. You may need to reset your preferences.", MessageBoxIcon.Warning);
                }
                key.Close();
            }
        }

        private void SaveEditorWindowSizeToRegistry()
        {
            if (_mainForm.WindowState == FormWindowState.Minimized)
            {
                // If the window is currently minimized, don't save any of its settings
                return;
            }

            RegistryKey key = Utilities.OpenAGSRegistryKey();
            if (key != null)
            {
                key.SetValue("MainWinMaximize", (_mainForm.WindowState == FormWindowState.Maximized) ? "1" : "0");
                if (_mainForm.WindowState == FormWindowState.Normal)
                {
                    key.SetValue("MainWinWidth", _mainForm.Width.ToString());
                    key.SetValue("MainWinHeight", _mainForm.Height.ToString());
                    key.SetValue("MainWinX", _mainForm.Left.ToString());
                    key.SetValue("MainWinY", _mainForm.Top.ToString());
                }
                /*int splitterX, splitterY;
                _mainForm.GetSplitterPositions(out splitterX, out splitterY);
                key.SetValue("MainWinSplitter1", splitterX.ToString());
                key.SetValue("MainWinSplitter2", splitterY.ToString());*/
                key.Close();
            }
        }

        private void InitializeColorTheme()
        {
            string colorTheme = Factory.AGSEditor.Preferences.EditorColorTheme;

            if (colorTheme.Equals("Vanilla"))
            {
                this.UserColorTheme = new VanillaTheme();
            }
            else if (colorTheme.Equals("Draconian"))
            {
                this.UserColorTheme = new DraconianTheme();
            }
            else
            {
                // TODO --- Load Custom Color theme
            }
        }

		void IGUIController.DrawSprite(Graphics g, int spriteNumber, int x, int y, int width, int height, bool centreHorizontally)
		{
			int spriteWidth = Factory.NativeProxy.GetRelativeSpriteWidth(spriteNumber) * 2;
			int spriteHeight = Factory.NativeProxy.GetRelativeSpriteHeight(spriteNumber) * 2;
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
