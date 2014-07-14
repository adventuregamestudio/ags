using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor.Components
{
    class ScriptsComponent : BaseComponentWithScripts<ScriptAndHeader, ScriptFolder>
    {
        private const string SCRIPTS_COMMAND_ID = "ScriptsCmd";
        private const string MENU_COMMAND_RENAME = "RenameScript";
        private const string MENU_COMMAND_DELETE = "DeleteScript";
        private const string MENU_COMMAND_NEW = "NewScript";
        private const string MENU_COMMAND_IMPORT = "ImportScript";
        private const string MENU_COMMAND_EXPORT = "ExportScript";
        private const string ICON_KEY = "ScriptIcon";

        private const string COMMAND_OPEN_GLOBAL_SCRIPT = "GoToGlobalScript";
        private const string COMMAND_OPEN_GLOBAL_HEADER = "GoToGlobalScriptHeader";

        private const string SCRIPT_MODULE_FILE_FILTER = "AGS script modules (*.scm)|*.scm";

        private delegate void CloseScriptEditor();

        private Dictionary<Script, ContentDocument> _editors;
        private ScriptEditor _lastActivated;
        private Timer _timer;
        private bool _timerActivateWindow = true;
        private EventHandler _panelClosedHandler;
        private string _rightClickedScript = null;
        private ProjectTreeItem _renamedTreeItem = null;

        public ScriptsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, SCRIPTS_COMMAND_ID)
        {
            _panelClosedHandler = new EventHandler(Script_PanelClosed);
            _timer = new Timer();
            _timer.Interval = 10;
            _timer.Tick += new EventHandler(timer_Tick);
            _editors = new Dictionary<Script, ContentDocument>();

            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("script.ico"));
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
            commands.Commands.Add(new MenuCommand(COMMAND_OPEN_GLOBAL_HEADER, "Open GlobalS&cript.ash", Keys.Control | Keys.Shift | Keys.H, "MenuIconGlobalHeader"));
            commands.Commands.Add(new MenuCommand(COMMAND_OPEN_GLOBAL_SCRIPT, "Open Glo&balScript.asc", Keys.Control | Keys.Shift | Keys.G, "MenuIconGlobalScript"));
            _guiController.AddMenuItems(this, commands);

            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Scripts", "ScriptsIcon");
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

        protected override void ItemCommandClick(string controlID)
        {
            if (controlID == MENU_COMMAND_RENAME)
            {
                _guiController.ProjectTree.BeginLabelEdit(this, ITEM_COMMAND_PREFIX + Path.GetFileNameWithoutExtension(_rightClickedScript));
            }
            else if (controlID == MENU_COMMAND_DELETE)
            {
                if (_guiController.ShowQuestion("Are you sure you want to delete this script and its header?") == DialogResult.Yes)
                {
                    ScriptAndHeader chosenItem = _agsEditor.CurrentGame.RootScriptFolder.GetScriptAndHeaderByName(_rightClickedScript, true);
                    DeleteSingleItem(chosenItem);
                }
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
                    ScriptAndHeader scripts = _agsEditor.CurrentGame.RootScriptFolder.GetScriptAndHeaderByName(_rightClickedScript, true);
                    Script script = scripts.Script;
                    Script header = scripts.Header;
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
                ScriptAndHeader scripts = new ScriptAndHeader(newHeader, newScript);
                string newNodeID = AddSingleItem(scripts);
                _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                RePopulateTreeView(GetNodeID(newScript));
                _guiController.ProjectTree.BeginLabelEdit(this, ITEM_COMMAND_PREFIX + newScript.NameForLabelEdit);
            }
            else if (controlID == COMMAND_OPEN_GLOBAL_HEADER)
            {
                CreateOrShowEditorForScript(Script.GLOBAL_HEADER_FILE_NAME);
            }
            else if (controlID == COMMAND_OPEN_GLOBAL_SCRIPT)
            {
                CreateOrShowEditorForScript(Script.GLOBAL_SCRIPT_FILE_NAME);
            }
            else if ((!controlID.StartsWith(NODE_ID_PREFIX_FOLDER)) &&
                     (controlID != TOP_LEVEL_COMMAND_ID))
            {
                string scriptName = controlID.Substring(ITEM_COMMAND_PREFIX.Length);
                CreateOrShowEditorForScript(scriptName);
            }
        }

        protected override void DeleteResourcesUsedByItem(ScriptAndHeader item)
        {
            DeleteScript(item);
        }

        private void DeleteScript(ScriptAndHeader chosenItem)
        {
            Script script = chosenItem.Script;
            Script header = chosenItem.Header;

            ContentDocument document;
            if (_editors.TryGetValue(script, out document))
            {
                _guiController.RemovePaneIfExists(document);
            }
            if (_editors.TryGetValue(header, out document))
            {
                _guiController.RemovePaneIfExists(document);
            }
            _agsEditor.DeleteFileOnDiskAndSourceControl(new string[] { script.FileName, header.FileName });
            _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
        }

        protected override ContentDocument GetDocument(ScriptEditor editor)
        {
            ContentDocument document;
            if (!_editors.TryGetValue(editor.Script, out document)) return null;
            return document;
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
                ContentDocument document;
                if (_editors.TryGetValue(header, out document))
                {
                    ((ScriptEditor)document.Control).SaveChanges();
                }
                if (_editors.TryGetValue(script, out document))
                {
                    ((ScriptEditor)document.Control).SaveChanges();
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
                ScriptAndHeader scripts = new ScriptAndHeader(newScripts[0], newScripts[1]);
                AddSingleItem(scripts);
                _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                RePopulateTreeView(GetNodeID(scripts));
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
                ContentDocument document = _editors[script];
                document.TreeNodeID = GetNodeID(script);
                _guiController.AddOrShowPane(document);
            }
            return editor;
        }

        private ScriptEditor CreateEditorForScript(Script chosenItem)
        {
            chosenItem.LoadFromDisk();
            ScriptEditor newEditor = new ScriptEditor(chosenItem, _agsEditor, ShowMatchingScriptOrHeader);
            newEditor.DockingContainer = new DockingContainer(newEditor);
            newEditor.IsModifiedChanged += new EventHandler(ScriptEditor_IsModifiedChanged);
            _editors[chosenItem] = new ContentDocument(newEditor, chosenItem.FileName, this, ICON_KEY, null);
            _editors[chosenItem].PanelClosed += _panelClosedHandler;
            _editors[chosenItem].ToolbarCommands = newEditor.ToolbarIcons;
            _editors[chosenItem].MainMenu = newEditor.ExtraMenu;
            if (!chosenItem.IsHeader)
            {
                _editors[chosenItem].SelectedPropertyGridObject = chosenItem;
            }
            return newEditor;
        }

        private ScriptEditor GetScriptEditor(string fileName, out Script script)
        {
            script = _agsEditor.CurrentGame.RootScriptFolder.GetScriptByFileName(fileName, true);
            if (script == null) return null;

            ContentDocument document;
            if (!_editors.TryGetValue(script, out document) || document.Control.IsDisposed)
            {
                CreateEditorForScript(script);
            }
            return (ScriptEditor)_editors[script].Control;
        }

        private ScriptEditor CreateOrShowEditorForScript(string scriptName)
        {
            return CreateOrShowEditorForScript(scriptName, true);
        }

        private ScriptEditor CreateOrShowEditorForScript(string scriptName, bool activateEditor)
        {
            Script chosenItem;
            if (!scriptName.Contains(".")) scriptName += ".asc";
            var scriptEditor = GetScriptEditor(scriptName, out chosenItem);
            if (chosenItem == null)
            {
                return null;
            }
            _lastActivated = scriptEditor;
            ContentDocument document = _editors[chosenItem];
            document.TreeNodeID = GetNodeID(chosenItem);
            _guiController.AddOrShowPane(document);
            if (activateEditor)
            {
                // Hideous hack -- we need to allow the current message to
                // finish processing before setting the focus to the
                // script window, or it will fail
                _timerActivateWindow = true;
                _timer.Start();
            }
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

            ScriptEditor editor = CreateOrShowEditorForScript(evArgs.FileName, evArgs.ActivateEditor);
            ZoomToCorrectPositionInScript(editor, evArgs);
        }

        private void GUIController_OnGetScript(string fileName, ref Script script)
        {
            Script chosenItem = _agsEditor.CurrentGame.RootScriptFolder.GetScriptByFileName(fileName, true);
            if (chosenItem == null)
            {
                return;
            }

            ContentDocument document;
            if (_editors.TryGetValue(chosenItem, out document))
            {
                ((ScriptEditor)document.Control).UpdateScriptObjectWithLatestTextInWindow();
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
            ContentDocument document;
            if (_editors.TryGetValue(script, out document))
            {
                ((ScriptEditor)document.Control).ScriptModifiedExternally();
            }
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

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(MENU_COMMAND_NEW, "New script", null));
            menu.Add(new MenuCommand(MENU_COMMAND_IMPORT, "Import script...", null));
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            // No more commands in this menu
        }

        protected override bool CreateItemsAtTop
        {
            get
            {
                return true;
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> contextMenu = base.GetContextMenu(controlID);
            if (controlID.StartsWith(ITEM_COMMAND_PREFIX) && !controlID.Contains(".ash") &&
                !controlID.Contains(".asc") && !IsFolderNode(controlID))
            {
                _rightClickedScript = controlID.Substring(ITEM_COMMAND_PREFIX.Length) + ".ash";
                contextMenu.Add(new MenuCommand(MENU_COMMAND_RENAME, "Rename", null));
                contextMenu.Add(new MenuCommand(MENU_COMMAND_DELETE, "Delete", null));
                contextMenu.Add(MenuCommand.Separator);
                contextMenu.Add(new MenuCommand(MENU_COMMAND_EXPORT, "Export...", null));

                if (_rightClickedScript == Script.GLOBAL_HEADER_FILE_NAME)
                {
                    foreach (MenuCommand command in contextMenu)
                    {
                        command.Enabled = false;
                    }
                }
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
                Dictionary<Script, ContentDocument>.ValueCollection.Enumerator enumerator = _editors.Values.GetEnumerator();
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

            foreach (KeyValuePair<Script, ContentDocument> pair in _editors)
            {
                Script script = pair.Key;
                if (pair.Value == document)
                {
                    _editors.Remove(script);
                    break;
                }
            }

        }

        private void ProjectTree_OnAfterLabelEdit(string commandID, ProjectTreeItem treeItem)
        {
            if (commandID.StartsWith(ITEM_COMMAND_PREFIX))
            {
                _timerActivateWindow = false;
                _rightClickedScript = commandID.Substring(ITEM_COMMAND_PREFIX.Length) + ".ash";
                _renamedTreeItem = treeItem;
                _timer.Start();
            }
        }

        private Script GetAssociatedScriptOrHeader(Script oneScript, string scriptName)
        {
            Script associatedScript = null;
            foreach (Script item in _agsEditor.CurrentGame.RootScriptFolder.AllScriptsFlat)
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

        private void ShowMatchingScriptOrHeader(Script script)
        {
            Script matchingScript = GetAssociatedScriptOrHeader(script, script.FileName);
            CreateOrShowEditorForScript(matchingScript.FileName);
        }

        private void ScriptRenamed(string oldScriptName, ProjectTreeItem renamedItem)
        {
            Script renamedScript = renamedItem.LabelTextDataSource as Script;
            if (renamedScript == null) return;
            if (renamedScript.FileName == oldScriptName)
            {
                // they didn't actually change the name
                RePopulateTreeView(GetNodeID(renamedScript));
                return;
            }

            if (!Utilities.DoesFileNameContainOnlyValidCharacters(renamedScript.FileName))
            {
                _guiController.ShowMessage("The file name '" + renamedScript.FileName + "' contains some invalid characters. You cannot use some characters like : and / in script file names.", MessageBoxIcon.Warning);
                renamedScript.FileName = oldScriptName;
                RePopulateTreeView(GetNodeID(renamedScript));
                return;
            }

            Script associatedScript = GetAssociatedScriptOrHeader(renamedScript, oldScriptName);
            if (associatedScript == null)
            {
                _guiController.ShowMessage("Internal error: Associated script or header could not be found", MessageBoxIcon.Error);
                return;
            }

            string newNameForAssociatedScript = renamedScript.NameForLabelEdit + Path.GetExtension(associatedScript.FileName);

            if (((File.Exists(renamedScript.FileName)) || (File.Exists(newNameForAssociatedScript))) &&
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

            ContentDocument document;
            if (_editors.TryGetValue(renamedScript, out document))
            {
                UpdateScriptWindowTitle((ScriptEditor)document.Control);
            }
            if (_editors.TryGetValue(associatedScript, out document))
            {
                UpdateScriptWindowTitle((ScriptEditor)document.Control);
            }

            RePopulateTreeView(GetNodeID(renamedScript));
        }

        protected override ProjectTreeItem CreateTreeItemForItem(ScriptAndHeader item)
        {
            ProjectTree treeController = _guiController.ProjectTree;
            ProjectTreeItem newItem = (ProjectTreeItem)treeController.AddTreeBranch(this, GetNodeID(item), item.Name, ICON_KEY);
            newItem.AllowDoubleClickWhenExpanding = true;

            const string editHeaderText = "Edit Header";
            const string editScriptText = "Edit Script";

            if (item.Name != Path.GetFileNameWithoutExtension(Script.GLOBAL_HEADER_FILE_NAME))
            {
                newItem.AllowLabelEdit = true;
                newItem.LabelTextProperty = item.Header.GetType().GetProperty("NameForLabelEdit");
                newItem.LabelTextDataSource = item.Header;
                treeController.AddTreeLeaf(this, GetNodeID(item.Header), editHeaderText, ICON_KEY);
                treeController.AddTreeLeaf(this, GetNodeID(item.Script), editScriptText, ICON_KEY);
            }
            else
            {
                treeController.AddTreeLeaf(this, GetNodeID(item.Header), editHeaderText, "MenuIconGlobalHeader");
                treeController.AddTreeLeaf(this, GetNodeID(item.Script), editScriptText, "MenuIconGlobalScript");
            }

            return newItem;
        }

        private string GetNodeID(Script script)
        {
            return ITEM_COMMAND_PREFIX + script.FileName;
        }

        private string GetNodeID(ScriptAndHeader scripts)
        {
            return ITEM_COMMAND_PREFIX + scripts.Name;
        }

        protected override bool CanFolderBeDeleted(ScriptFolder folder)
        {
            return folder.GetScriptByFileName(Script.GLOBAL_HEADER_FILE_NAME, true) == null;
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its scripts?" + Environment.NewLine + Environment.NewLine + "You won't be able to recover them afterwards.";
        }

        protected override ScriptFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootScriptFolder;
        }
    }
}
