using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class ScriptsComponent : BaseComponentWithScripts
    {
        private const string ROOT_COMMAND = "ScriptsCmd";
        private const string MENU_COMMAND_RENAME = "RenameScript";
        private const string MENU_COMMAND_DELETE = "DeleteScript";
        private const string MENU_COMMAND_NEW = "NewScript";
        private const string MENU_COMMAND_IMPORT = "ImportScript";
        private const string MENU_COMMAND_EXPORT = "ExportScript";
        private const string MENU_COMMAND_MOVE_UP = "MoveScriptUp";
        private const string MENU_COMMAND_MOVE_DOWN = "MoveScriptDown";

		private const string COMMAND_OPEN_GLOBAL_SCRIPT = "GoToGlobalScript";
		private const string COMMAND_OPEN_GLOBAL_HEADER = "GoToGlobalScriptHeader";

        private const string SCRIPT_MODULE_FILE_FILTER = "AGS script modules (*.scm)|*.scm";

        private Dictionary<Script,ContentDocument> _editors;
        private ScriptEditor _lastActivated;
        private Timer _timer;
        private bool _timerActivateWindow = true;
        private EventHandler _panelClosedHandler;
        private string _rightClickedScript = null;
        private ProjectTreeItem _renamedTreeItem = null;

        public ScriptsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _panelClosedHandler = new EventHandler(Script_PanelClosed);
            _timer = new Timer();
            _timer.Interval = 10;
            _timer.Tick += new EventHandler(timer_Tick);
            _editors = new Dictionary<Script, ContentDocument>();

            _guiController.RegisterIcon("ScriptIcon", Resources.ResourceManager.GetIcon("script.ico"));
            _guiController.RegisterIcon("ScriptsIcon", Resources.ResourceManager.GetIcon("scripts.ico"));
            _guiController.RegisterIcon("CutIcon", Resources.ResourceManager.GetIcon("cut.ico"));
            _guiController.RegisterIcon("CopyIcon", Resources.ResourceManager.GetIcon("copy.ico"));
            _guiController.RegisterIcon("PasteIcon", Resources.ResourceManager.GetIcon("paste.ico"));
            _guiController.RegisterIcon("UndoIcon", Resources.ResourceManager.GetIcon("undo.ico"));
            _guiController.RegisterIcon("RedoIcon", Resources.ResourceManager.GetIcon("redo.ico"));
            _guiController.RegisterIcon("CutMenuIcon", Resources.ResourceManager.GetIcon("menu_edit_cut.ico"));
            _guiController.RegisterIcon("CopyMenuIcon", Resources.ResourceManager.GetIcon("menu_edit_copy.ico"));
            _guiController.RegisterIcon("PasteMenuIcon", Resources.ResourceManager.GetIcon("menu_edit_paste.ico"));
            _guiController.RegisterIcon("UndoMenuIcon", Resources.ResourceManager.GetIcon("menu_edit_undo.ico"));
            _guiController.RegisterIcon("RedoMenuIcon", Resources.ResourceManager.GetIcon("menu_edit_redo.ico"));
            _guiController.RegisterIcon("FindMenuIcon", Resources.ResourceManager.GetIcon("find.ico"));
            _guiController.RegisterIcon("FindNextMenuIcon", Resources.ResourceManager.GetIcon("findnext.ico"));
            _guiController.RegisterIcon("ShowAutocompleteMenuIcon", Resources.ResourceManager.GetIcon("showautocomplete.ico"));
            _guiController.RegisterIcon("ToggleBreakpointMenuIcon", Resources.ResourceManager.GetIcon("togglebreakpoint.ico"));
            _guiController.RegisterIcon("MenuIconGlobalScript", Resources.ResourceManager.GetIcon("menu_file_glscript-asc.ico"));
            _guiController.RegisterIcon("MenuIconGlobalHeader", Resources.ResourceManager.GetIcon("menu_file_glscript-ash.ico"));

            MenuCommands commands = new MenuCommands(GUIController.FILE_MENU_ID, 250);
            commands.Commands.Add(new MenuCommand(COMMAND_OPEN_GLOBAL_HEADER, "Open GlobalS&cript.ash", Keys.Control | Keys.H, "MenuIconGlobalHeader"));
            commands.Commands.Add(new MenuCommand(COMMAND_OPEN_GLOBAL_SCRIPT, "Open Glo&balScript.asc", Keys.Control | Keys.G, "MenuIconGlobalScript"));
            _guiController.AddMenuItems(this, commands);

            _guiController.ProjectTree.AddTreeRoot(this, ROOT_COMMAND, "Scripts", "ScriptsIcon");
            RePopulateTreeView(null);
            _guiController.OnZoomToFile += new GUIController.ZoomToFileHandler(GUIController_OnZoomToFile);
            _guiController.OnGetScript += new GUIController.GetScriptHandler(GUIController_OnGetScript);
            _guiController.OnScriptChanged += new GUIController.ScriptChangedHandler(GUIController_OnScriptChanged);
            _guiController.OnGetScriptEditorControl += new GUIController.GetScriptEditorControlHandler(_guiController_OnGetScriptEditorControl);
            _guiController.ProjectTree.OnAfterLabelEdit += new ProjectTree.AfterLabelEditHandler(ProjectTree_OnAfterLabelEdit);
        }

        private void _guiController_OnGetScriptEditorControl(GetScriptEditorControlEventArgs evArgs)
        {
            var scriptEditor = GetScriptEditor(evArgs.ScriptFileName, evArgs.ShowEditor);
            if (scriptEditor != null)
            {
                evArgs.ScriptEditor = scriptEditor.ScriptEditorControl;
            }
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Scripts; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == MENU_COMMAND_RENAME)
            {
                _guiController.ProjectTree.BeginLabelEdit(this, "Scr" + _rightClickedScript);
            }
            else if (controlID == MENU_COMMAND_DELETE)
            {
                if (_guiController.ShowQuestion("Are you sure you want to delete this script and its header?") == DialogResult.Yes)
                {
                    Script chosenItem = _agsEditor.CurrentGame.Scripts.GetScriptByFilename(_rightClickedScript);
                    Script header = GetAssociatedScriptOrHeader(chosenItem, _rightClickedScript);

					if (_editors.ContainsKey(chosenItem))
					{
						_guiController.RemovePaneIfExists(_editors[chosenItem]);
					}
					if (_editors.ContainsKey(header))
					{
						_guiController.RemovePaneIfExists(_editors[header]);
					}

                    _agsEditor.CurrentGame.Scripts.Remove(chosenItem);
                    _agsEditor.CurrentGame.Scripts.Remove(header);
					_agsEditor.DeleteFileOnDiskAndSourceControl(new string[] { chosenItem.FileName, header.FileName });
					_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                    RePopulateTreeView(null);
                }
            }
            else if (controlID == MENU_COMMAND_MOVE_UP)
            {
                Script chosenItem = _agsEditor.CurrentGame.Scripts.GetScriptByFilename(_rightClickedScript);
                _agsEditor.CurrentGame.Scripts.MoveScriptAndHeaderUp(chosenItem);
                RePopulateTreeView(chosenItem);
            }
            else if (controlID == MENU_COMMAND_MOVE_DOWN)
            {
                Script chosenItem = _agsEditor.CurrentGame.Scripts.GetScriptByFilename(_rightClickedScript);
                _agsEditor.CurrentGame.Scripts.MoveScriptAndHeaderDown(chosenItem);
                RePopulateTreeView(chosenItem);
            }
            else if (controlID == MENU_COMMAND_IMPORT)
            {
                string fileName = _guiController.ShowOpenFileDialog("Select script to import...", SCRIPT_MODULE_FILE_FILTER);
                if (fileName != null)
                {
                    ImportScriptModule(fileName);
                }
            }
            else if (controlID == MENU_COMMAND_EXPORT)
            {
                string fileName = _guiController.ShowSaveFileDialog("Export script as...", SCRIPT_MODULE_FILE_FILTER);
                if (fileName != null)
                {
                    Script script = _agsEditor.CurrentGame.Scripts.GetScriptByFilename(_rightClickedScript);
                    Script header = GetAssociatedScriptOrHeader(script, _rightClickedScript);
                    if (script.IsHeader)
                    {
                        // they selected the header to export
                        Script temp = script;
                        script = header;
                        header = temp;
                    }
                    ExportScriptModule(header, script, fileName);
                }
            }
            else if (controlID == MENU_COMMAND_NEW)
            {
                string newFileName = FindFirstAvailableFileName("NewScript");
                Script newScript = new Script(newFileName + ".asc", "// new module script\r\n", false);
                Script newHeader = new Script(newFileName + ".ash", "// new module header\r\n", true);
                newScript.Modified = true;
                newScript.SaveToDisk();
                newHeader.Modified = true;
                newHeader.SaveToDisk();
                _agsEditor.CurrentGame.Scripts.AddAtTop(newScript);
                _agsEditor.CurrentGame.Scripts.AddAtTop(newHeader);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                RePopulateTreeView(newScript);
                _guiController.ProjectTree.BeginLabelEdit(this, "Scr" + newScript.FileName);
            }
			else if (controlID == COMMAND_OPEN_GLOBAL_HEADER)
			{
				CreateOrShowEditorForScript(Script.GLOBAL_HEADER_FILE_NAME);
			}
			else if (controlID == COMMAND_OPEN_GLOBAL_SCRIPT)
			{
				CreateOrShowEditorForScript(Script.GLOBAL_SCRIPT_FILE_NAME);
			}
			else if (controlID != ROOT_COMMAND)
			{
				string scriptName = controlID.Substring(3);
				CreateOrShowEditorForScript(scriptName);
			}
        }

        private string FindFirstAvailableFileName(string prefix)
        {
            int attempt = 0;
            string newFileName;
            do
            {
                newFileName = prefix + ((attempt > 0) ? attempt.ToString() : string.Empty);
                attempt++;
            }
            while ((File.Exists(newFileName + ".asc")) ||
                   (File.Exists(newFileName + ".ash")));

            return newFileName;
        }

        private void ExportScriptModule(Script header, Script script, string fileName)
        {
            try
            {
                if (_editors.ContainsKey(header))
                {
                    ((ScriptEditor)_editors[header].Control).SaveChanges();
                }
                if (_editors.ContainsKey(script))
                {
                    ((ScriptEditor)_editors[script].Control).SaveChanges();
                }

                ImportExport.ExportScriptModule(header, script, fileName);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("An error occurred trying to export the script module. The error details are below." + Environment.NewLine + Environment.NewLine + ex.ToString(), MessageBoxIcon.Warning);
            }
        }

        private void ImportScriptModule(string fileName)
        {
            try
            {
                string destFileName = FindFirstAvailableFileName(Path.GetFileNameWithoutExtension(fileName));
                List<Script> newScripts = ImportExport.ImportScriptModule(fileName);
                newScripts[0].FileName = destFileName + ".ash";
                newScripts[1].FileName = destFileName + ".asc";
                newScripts[0].Modified = true;
                newScripts[1].Modified = true;
                newScripts[0].SaveToDisk();
                newScripts[1].SaveToDisk();
                _agsEditor.CurrentGame.Scripts.AddAtTop(newScripts[1]);
                _agsEditor.CurrentGame.Scripts.AddAtTop(newScripts[0]);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                RePopulateTreeView(newScripts[1]);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("An error occurred trying to import the script module. The error details are below." + Environment.NewLine + Environment.NewLine + ex.ToString(), MessageBoxIcon.Warning);
            }
        }

        private IScriptEditor GetScriptEditor(string fileName, bool showEditor)
        {
            Script script;
            ScriptEditor editor = GetScriptEditor(fileName, out script);
            if ((showEditor) && (editor != null))
            {
                _guiController.AddOrShowPane(_editors[script]);
            }
            return editor;
        }

        private void CreateEditorForScript(Script chosenItem)
        {
            chosenItem.LoadFromDisk();
            ScriptEditor newEditor = new ScriptEditor(chosenItem, _agsEditor);
            newEditor.IsModifiedChanged += new EventHandler(ScriptEditor_IsModifiedChanged);
            _editors.Add(chosenItem, new ContentDocument(newEditor, chosenItem.FileName, this, null));
            _editors[chosenItem].PanelClosed += _panelClosedHandler;
            _editors[chosenItem].ToolbarCommands = newEditor.ToolbarIcons;
            _editors[chosenItem].MainMenu = newEditor.ExtraMenu;
            if (!chosenItem.IsHeader)
            {
                _editors[chosenItem].SelectedPropertyGridObject = chosenItem;
            }
        }

        private ScriptEditor GetScriptEditor(string fileName, out Script script)
        {
            script = _agsEditor.CurrentGame.Scripts.GetScriptByFilename(fileName);
            if (script == null)
            {
                return null;
            }
            if (!_editors.ContainsKey(script))
            {
                CreateEditorForScript(script);
            }
            return (ScriptEditor)_editors[script].Control;
        }
        
        private ScriptEditor CreateOrShowEditorForScript(string scriptName)
        {
            Script chosenItem;
            var scriptEditor = GetScriptEditor(scriptName, out chosenItem);
            if (chosenItem == null)
            {
                return null;
            }
            _lastActivated = scriptEditor;
            _guiController.AddOrShowPane(_editors[chosenItem]);
            // Hideous hack -- we need to allow the current message to
            // finish processing before setting the focus to the
            // script window, or it will fail
            _timerActivateWindow = true;
            _timer.Start();
            return _lastActivated;
        }

        private void RemoveExecutionPointFromAllScripts()
        {
            foreach (ContentDocument doc in _editors.Values)
            {
                ((ScriptEditor)doc.Control).RemoveExecutionPointMarker();
            }
        }

        private void GUIController_OnZoomToFile(ZoomToFileEventArgs evArgs)
        {
            if (evArgs.IsDebugExecutionPoint)
            {
                RemoveExecutionPointFromAllScripts();
            }

			if (evArgs.FileName == Tasks.AUTO_GENERATED_HEADER_NAME)
			{
				_guiController.ShowMessage("The error was within an automatically generated script file, so you cannot edit the script. The most likely cause of errors here is that two things in your game have the same name. For example, two characters or two fonts with the same script name could cause this error. Please consult the error message for more clues.", MessageBoxIcon.Warning);
				return;
			}

            ScriptEditor editor = CreateOrShowEditorForScript(evArgs.FileName);
            ZoomToCorrectPositionInScript(editor, evArgs);
        }

        private void GUIController_OnGetScript(string fileName, ref Script script)
        {
            Script chosenItem = _agsEditor.CurrentGame.Scripts.GetScriptByFilename(fileName);
            if (chosenItem == null)
            {
                return;
            }

            if (_editors.ContainsKey(chosenItem))
            {
                ((ScriptEditor)_editors[chosenItem].Control).UpdateScriptObjectWithLatestTextInWindow();
            }
            else
            {
                // The fact that this OnGetScript event has been fired implies
                // that something is about to modify it, so make sure it is
                // loaded into memory in order to accept changes
                CreateEditorForScript(chosenItem);
            }

            script = chosenItem;
        }

        private void GUIController_OnScriptChanged(Script script)
        {
            if (_editors.ContainsKey(script))
            {
                ((ScriptEditor)_editors[script].Control).ScriptModifiedExternally();
            }
        }

        private void UpdateScriptWindowTitle(ScriptEditor editor)
        {
            string newTitle = editor.Script.FileName + (editor.IsModified ? " *" : "");
            _editors[editor.Script].Name = newTitle;
            _guiController.DocumentTitlesChanged();
        }

        private void ScriptEditor_IsModifiedChanged(object sender, EventArgs e)
        {
            ScriptEditor sendingPane = (ScriptEditor)sender;
            UpdateScriptWindowTitle(sendingPane);
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            _timer.Stop();
            if (_timerActivateWindow)
            {
                _lastActivated.ActivateTextEditor();
            }
            else
            {
                ScriptRenamed(_rightClickedScript, _renamedTreeItem);
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            List<MenuCommand> contextMenu = new List<MenuCommand>();
            if (controlID != ROOT_COMMAND)
            {
                _rightClickedScript = controlID.Substring(3);
                contextMenu.Add(new MenuCommand(MENU_COMMAND_RENAME, "Rename", null));
                contextMenu.Add(new MenuCommand(MENU_COMMAND_DELETE, "Delete", null));
                contextMenu.Add(new MenuCommand(MENU_COMMAND_MOVE_UP, "Move Up", null));
                contextMenu.Add(new MenuCommand(MENU_COMMAND_MOVE_DOWN, "Move Down", null));
                contextMenu.Add(MenuCommand.Separator);
                contextMenu.Add(new MenuCommand(MENU_COMMAND_EXPORT, "Export...", null));

                if ((_rightClickedScript == Script.GLOBAL_HEADER_FILE_NAME) ||
                    (_rightClickedScript == Script.GLOBAL_SCRIPT_FILE_NAME))
                {
                    foreach (MenuCommand command in contextMenu)
                    {
                        command.Enabled = false;
                    }
                }
            }
            else
            {
                contextMenu.Add(new MenuCommand(MENU_COMMAND_NEW, "New script", null));
                contextMenu.Add(new MenuCommand(MENU_COMMAND_IMPORT, "Import script...", null));
            }
            return contextMenu;
        }

        public override void BeforeSaveGame()
        {
            foreach (ContentDocument doc in _editors.Values)
            {
                ((ScriptEditor)doc.Control).SaveChanges();
            }
        }

        public override void RefreshDataFromGame()
        {
            // The Script_PanelClosed event will get called as we remove each
            // one, so we need to be careful with the enumerator
            while (_editors.Values.Count > 0) 
            {
                Dictionary<Script,ContentDocument>.ValueCollection.Enumerator enumerator = _editors.Values.GetEnumerator();
                enumerator.MoveNext();
                _guiController.RemovePaneIfExists(enumerator.Current);
            }
            _editors.Clear();

            RePopulateTreeView(null);
        }

        private void Script_PanelClosed(object sender, EventArgs e)
        {
            // When they close the script window, dispose it.
            // This ensures that when they open it again they
            // get a clean version
            ContentDocument document = ((ContentDocument)sender);
            document.PanelClosed -= _panelClosedHandler;
            document.Dispose();

            foreach (Script script in _editors.Keys)
            {
                if (_editors[script] == document)
                {
                    _editors.Remove(script);
                    break;
                }
            }
            
        }

        private void ProjectTree_OnAfterLabelEdit(string commandID, ProjectTreeItem treeItem)
        {
            if (commandID.StartsWith("Scr"))
            {
                _timerActivateWindow = false;
                _rightClickedScript = commandID.Substring(3);
                _renamedTreeItem = treeItem;
                _timer.Start();
            }
        }

        private Script GetAssociatedScriptOrHeader(Script oneScript, string scriptName)
        {
            Script associatedScript = null;
            foreach (Script item in _agsEditor.CurrentGame.Scripts)
            {
                if ((item.NameForLabelEdit == Path.GetFileNameWithoutExtension(scriptName)) &&
                    (item != oneScript))
                {
                    associatedScript = item;
                    break;
                }
            }
            return associatedScript;
        }

        private bool FileNamesDifferOnlyByCase(string fileName1, string fileName2)
        {
            return (fileName1.ToLower() == fileName2.ToLower());
        }

        private void ScriptRenamed(string oldScriptName, ProjectTreeItem renamedItem)
        {
            Script renamedScript = (Script)renamedItem.LabelTextDataSource;
            if (renamedScript.FileName == oldScriptName)
            {
                // they didn't actually change the name
                RePopulateTreeView(renamedScript);
                return;
            }

            Script associatedScript = GetAssociatedScriptOrHeader(renamedScript, oldScriptName);
            if (associatedScript == null)
            {
                _guiController.ShowMessage("Internal error: Associated script or header could not be found", MessageBoxIcon.Error);
                return;
            }

            string newNameForAssociatedScript = renamedScript.NameForLabelEdit + Path.GetExtension(associatedScript.FileName);

            if (!Utilities.DoesFileNameContainOnlyValidCharacters(renamedScript.FileName))
            {
                _guiController.ShowMessage("The file name '" + renamedScript.FileName + "' contains some invalid characters. You cannot use some characters like : and / in script file names.", MessageBoxIcon.Warning);
                renamedScript.FileName = oldScriptName;
            }
            else if (((File.Exists(renamedScript.FileName)) || (File.Exists(newNameForAssociatedScript))) &&
                    (!FileNamesDifferOnlyByCase(renamedScript.FileName, oldScriptName)))
            {
                _guiController.ShowMessage("A file with the name '" + renamedScript.FileName + "' already exists.", MessageBoxIcon.Warning);
                renamedScript.FileName = oldScriptName;
            }
            else
            {
                _agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldScriptName, renamedScript.FileName);
				_agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(associatedScript.FileName, newNameForAssociatedScript);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                associatedScript.FileName = newNameForAssociatedScript;
            }

            if (_editors.ContainsKey(renamedScript))
            {
                UpdateScriptWindowTitle((ScriptEditor)_editors[renamedScript].Control);
            }
            if (_editors.ContainsKey(associatedScript))
            {
                UpdateScriptWindowTitle((ScriptEditor)_editors[associatedScript].Control);
            }

            RePopulateTreeView(renamedScript);
        }

        private void RePopulateTreeView(Script scriptToSelect)
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, ROOT_COMMAND);
            _guiController.ProjectTree.StartFromNode(this, ROOT_COMMAND);
            foreach (Script item in _agsEditor.CurrentGame.Scripts)
            {
                string iconKey = "ScriptIcon";
                bool allowLabelEdit = true;

                if (item.FileName == Script.GLOBAL_HEADER_FILE_NAME)
                {
                    iconKey = "MenuIconGlobalHeader";
                    allowLabelEdit = false;
                }
                else if (item.FileName == Script.GLOBAL_SCRIPT_FILE_NAME)
                {
                    iconKey = "MenuIconGlobalScript";
                    allowLabelEdit = false;
                }

                ProjectTreeItem newItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf(this, "Scr" + item.FileName, item.FileName, iconKey);

                if (allowLabelEdit)
                {
                    newItem.AllowLabelEdit = true;
                    newItem.LabelTextProperty = item.GetType().GetProperty("NameForLabelEdit");
                    newItem.LabelTextDataSource = item;
                }
            }

            if (scriptToSelect != null)
            {
                _guiController.ProjectTree.SelectNode(this, "Scr" + scriptToSelect.FileName);
            }
            else if (_editors.ContainsValue(_guiController.ActivePane))
            {
                ScriptEditor editor = (ScriptEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, "Scr" + editor.Script.FileName);
            }
            else if (_agsEditor.CurrentGame.Scripts.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "Scr" + _agsEditor.CurrentGame.Scripts[0].FileName);
            }
        }
    }
}
