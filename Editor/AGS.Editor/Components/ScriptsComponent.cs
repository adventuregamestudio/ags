using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Editor.Preferences;
using AGS.Editor.Utils;
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
        private const string MENU_COMMAND_ADD_EXISTING = "AddExistingScript";
        private const string ICON_KEY = "ScriptIcon";
        
		private const string COMMAND_OPEN_GLOBAL_SCRIPT = "GoToGlobalScript";
		private const string COMMAND_OPEN_GLOBAL_HEADER = "GoToGlobalScriptHeader";

        private const string SCRIPT_MODULE_FILE_FILTER = "AGS script modules (*.scm)|*.scm";
        private const string SCRIPT_FILES_FILE_FILTER = "AGS script or header (*.asc;*.ash)|*.asc;*.ash";

        private delegate void CloseScriptEditor();

        private Dictionary<Script,ContentDocument> _editors;
        private EventHandler _panelClosedHandler;
        private string _rightClickedScript = null;

        public ScriptsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, SCRIPTS_COMMAND_ID)
        {
            _panelClosedHandler = new EventHandler(Script_PanelClosed);
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
            _guiController.RegisterIcon("WordWrapIcon", Resources.ResourceManager.GetIcon("word-wrap.ico"));

            MenuCommands commands = new MenuCommands(GUIController.FILE_MENU_ID, 250);
            commands.Commands.Add(new MenuCommand(COMMAND_OPEN_GLOBAL_HEADER, "Open GlobalS&cript.ash", Keys.Control | Keys.Shift | Keys.H, "MenuIconGlobalHeader"));
            commands.Commands.Add(new MenuCommand(COMMAND_OPEN_GLOBAL_SCRIPT, "Open Glo&balScript.asc", Keys.Control | Keys.Shift | Keys.G, "MenuIconGlobalScript"));
            _guiController.AddMenuItems(this, commands);

            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Scripts", "ScriptsIcon");
            RePopulateTreeView(null);
            _guiController.OnZoomToFile += GUIController_OnZoomToFile;
            _guiController.OnGetScript += GUIController_OnGetScript;
            _guiController.OnScriptChanged += GUIController_OnScriptChanged;
            _guiController.OnGetScriptEditorControl += _guiController_OnGetScriptEditorControl;
            _guiController.ProjectTree.OnAfterLabelEdit += ProjectTree_OnAfterLabelEdit;

            Factory.Events.GamePostLoad += Events_GamePostLoad;
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
                    ScriptListTypeConverter.SetScriptList(Factory.AGSEditor.CurrentGame.ScriptsAndHeaders);
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
            else if (controlID == MENU_COMMAND_ADD_EXISTING)
            {
                string currentGameDir = Factory.AGSEditor.CurrentGame.DirectoryPath;
                string[] fileNames = _guiController.ShowOpenFileDialogMultipleFiles("Select script/header pair to add...", SCRIPT_FILES_FILE_FILTER, currentGameDir);
                
                if (fileNames.Length > 0)
                {
                    AddScriptModules(fileNames);
                }
            }
            else if (controlID == MENU_COMMAND_NEW)
            {
                var scripts = AddNewScript("NewScript", "// new module header\r\n", "// new module script\r\n");
                AddSingleItem(scripts);
                ScriptListTypeConverter.SetScriptList(Factory.AGSEditor.CurrentGame.ScriptsAndHeaders);
                _guiController.ProjectTree.BeginLabelEdit(this, ITEM_COMMAND_PREFIX + scripts.Script.NameForLabelEdit);
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

        /// <summary>
        /// Adds a new script module (header/script pair) item to the project,
        /// finds the first available name based on requested one,
        /// assigns header and body text. Returns added Script object.
        /// </summary>
        public ScriptAndHeader AddNewScript(string scriptName, string headerText, string scriptText, bool insertOnTop)
        {
            var scripts = AddNewScript(scriptName, headerText, scriptText);
            if (insertOnTop)
                _agsEditor.CurrentGame.RootScriptFolder.Items.Insert(0, scripts);
            else
                _agsEditor.CurrentGame.RootScriptFolder.Items.Add(scripts);
            RePopulateTreeView();
            return scripts;
        }

        private ScriptAndHeader AddNewScript(string scriptName, string headerText, string scriptText)
        {
            string newFileName = FindFirstAvailableFileName(scriptName);
            Script newScript = new Script(newFileName + ".asc", scriptText, false);
            Script newHeader = new Script(newFileName + ".ash", headerText, true);
            newScript.Modified = true;
            newScript.SaveToDisk();
            newHeader.Modified = true;
            newHeader.SaveToDisk();
            _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
            return new ScriptAndHeader(newHeader, newScript);
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
            _agsEditor.DeleteFileOnDisk(new string[] { script.FileName, header.FileName });
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

                ImportExport.ExportScriptModule(header, script, fileName, _agsEditor.CurrentGame.TextEncoding);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("An error occurred trying to export the script module. The error details are below." + Environment.NewLine + Environment.NewLine + ex.ToString(), MessageBoxIcon.Warning);
            }
        }

        private List<ExistingScriptHeaderToAdd> GetNormalizedScriptHeaderPairs(string[] fileNames, CompileMessages errors)
        {
            string[] uniqueFilenames = fileNames.Distinct().ToArray();
            string[] deleted;

            string[] validFilenames = ScriptFileUtilities.FilterNonScriptFileNames(uniqueFilenames, out deleted);
            foreach(var d in deleted)
                errors.Add(new CompileWarning($"Not .asc or .ash extension in file \"{d}\"."));
            if (validFilenames.Length == 0)
            {
                return new List<ExistingScriptHeaderToAdd>();
            }

            string[] regularScriptFilenames = ScriptFileUtilities.FilterOutRoomScriptFileNames(validFilenames, out deleted);
            foreach (var d in deleted)
                errors.Add(new CompileWarning($"Cannot import room script \"{d}\" by itself, only script modules can be imported."));
            if (regularScriptFilenames.Length == 0)
            {
                return new List<ExistingScriptHeaderToAdd>();
            }

            ScriptsAndHeaders gameScripts = Factory.AGSEditor.CurrentGame.ScriptsAndHeaders;
            string[] relativeFilenames = Utilities.GetRelativeToProjectPath(regularScriptFilenames);
            string[] notInProjectFilenames = ScriptFileUtilities.FilterAlreadyInGameScripts(relativeFilenames, gameScripts, out deleted);
            foreach (var d in deleted)
                errors.Add(new CompileWarning($"Script file \"{d}\" is already in the game project."));
            if (notInProjectFilenames.Length == 0)
            {
                return new List<ExistingScriptHeaderToAdd>();
            }

            List<Tuple<string, string>> toBeAddedPairs = ScriptFileUtilities.PairHeadersAndScriptFiles(notInProjectFilenames);

            List<ExistingScriptHeaderToAdd> scriptsToAdd = new List<ExistingScriptHeaderToAdd>();
            foreach(var pair in toBeAddedPairs)
            {
                ExistingScriptHeaderToAdd scriptToAdd;
                scriptToAdd.Header.SrcFileName = pair.Item1;
                scriptToAdd.Script.SrcFileName = pair.Item2;

                // NOTE: AGS Editor currently has all non-room script files at the project root
                // we will guess that the destination file is simply the basename, but we will
                // need to check later if these already match an existing file in the project root.
                scriptToAdd.Header.DstFileName = Path.GetFileName(scriptToAdd.Header.SrcFileName);
                scriptToAdd.Script.DstFileName = Path.GetFileName(scriptToAdd.Script.SrcFileName);

                scriptsToAdd.Add(scriptToAdd);
            }

            return scriptsToAdd;
        }

        private int NewUniqueKey()
        {
            ScriptsAndHeaders gameScripts = Factory.AGSEditor.CurrentGame.ScriptsAndHeaders;
            int uniqueKey;
            do
            {
                uniqueKey = new Random().Next(Int32.MaxValue);
            } while (!ScriptFileUtilities.IsKeyUnique(uniqueKey, gameScripts));
            return uniqueKey;
        }

        /// <summary>
        /// Ensures uniqueness of file names in the game project, by fixing destination file names for script-header list.
        /// </summary>
        /// <param name="scriptsToAdd">List of script-header pairs to fix destination file names for.</param>
        /// <returns>A list of fixed existing script-header pairs where destination file names are updated if necessary.</returns>
        private List<ExistingScriptHeaderToAdd> FixDestinationFileNames(List<ExistingScriptHeaderToAdd> scriptsToAdd)
        {
            List<ExistingScriptHeaderToAdd> fixedScripts = new List<ExistingScriptHeaderToAdd>();
            foreach (var pair in scriptsToAdd)
            {
                string headerSrc = pair.Header.SrcFileName;
                string scriptSrc = pair.Script.SrcFileName;
                string headerDst = pair.Header.DstFileName;
                string scriptDst = pair.Script.DstFileName;

                string fileName = string.IsNullOrEmpty(headerDst) ? scriptDst : headerDst;
                string destFileName = Path.GetFileNameWithoutExtension(fileName);
                headerDst = destFileName + ".ash";
                scriptDst = destFileName + ".asc";

                bool headerRequireNewName = !string.IsNullOrEmpty(headerSrc) && (headerSrc != headerDst);
                bool scriptRequireNewName = !string.IsNullOrEmpty(scriptSrc) && (scriptSrc != scriptDst);

                if (headerRequireNewName || scriptRequireNewName)
                {
                    destFileName = FindFirstAvailableFileName(destFileName);
                }

                ExistingScriptHeaderToAdd scriptToAdd;
                scriptToAdd.Header.SrcFileName = pair.Header.SrcFileName;
                scriptToAdd.Script.SrcFileName = pair.Script.SrcFileName;
                scriptToAdd.Header.DstFileName = destFileName + ".ash";
                scriptToAdd.Script.DstFileName = destFileName + ".asc";
                fixedScripts.Add(scriptToAdd);
            }

            return fixedScripts;
        }

        /// <summary>
        /// Ensure files are not overwritten unnecessarily. 
        /// If a pair (script or header) is missing from file list but is present in the game directory, use it instead of overwriting with empty file.
        /// </summary>
        /// <param name="scriptsToAdd">List of existing script-header pairs to check and possibly fix.</param>
        /// <returns>A new list script-header pairs, where if files were missing and one if they exist at the destination, they are safely added.</returns>
        private List<ExistingScriptHeaderToAdd> SafelyAddMissingPairIfExists(List<ExistingScriptHeaderToAdd> scriptsToAdd)
        {
            List<ExistingScriptHeaderToAdd> fixedScripts = new List<ExistingScriptHeaderToAdd>();
            foreach (var pair in scriptsToAdd)
            {
                string headerSrc = pair.Header.SrcFileName;
                string scriptSrc = pair.Script.SrcFileName;
                string headerDst = pair.Header.DstFileName;
                string scriptDst = pair.Script.DstFileName;

                if(!string.IsNullOrEmpty(headerSrc) && !string.IsNullOrEmpty(scriptSrc))
                {
                    // both scripts are present at source, proceed as all is good
                    fixedScripts.Add(pair);
                    continue;
                }

                // if header is empty at source we would create a new empty header at destination
                // but if that file already exists, it's better we do not erase it!
                if (string.IsNullOrEmpty(headerSrc) && File.Exists(headerDst))
                {
                    headerSrc = headerDst;
                }

                // do the same for the script file
                if (string.IsNullOrEmpty(scriptSrc) && File.Exists(scriptDst))
                {
                    scriptSrc = scriptDst;
                }

                ExistingScriptHeaderToAdd fixedPair;
                fixedPair.Header.SrcFileName = headerSrc;
                fixedPair.Script.SrcFileName = scriptSrc;
                fixedPair.Header.DstFileName = headerDst;
                fixedPair.Script.DstFileName = scriptDst;
                fixedScripts.Add(fixedPair);
            }

            return fixedScripts;
        }

        private static string ReadScriptFile(Encoding enc, string fileName, string placeHolderText)
        {
            if (string.IsNullOrEmpty(fileName))
            {
                return placeHolderText;
            }

            int fileLength = new Func<int>(() =>
            {
                var fInfo = new FileInfo(fileName);
                return (int)fInfo.Length;
            })();

            BinaryReader reader = new BinaryReader(new FileStream(fileName, FileMode.Open, FileAccess.Read));
            byte[] textBytes = reader.ReadBytes(fileLength);
            reader.Close();            

            return enc.GetString(textBytes);
        }

        // returns added script NodeID
        private string AddExistingScriptFile(ExistingScriptHeaderToAdd scriptToAdd, CompileMessages errors)
        {
            try
            {
                Encoding enc = _agsEditor.CurrentGame.TextEncoding;
                string name = Path.GetFileNameWithoutExtension(scriptToAdd.Header.DstFileName);
                string headerSrcFileName = scriptToAdd.Header.SrcFileName;
                string scriptSrcFileName = scriptToAdd.Script.SrcFileName;
                int uniqueKey = NewUniqueKey();

                ScriptModuleDef module;
                module.Author = string.Empty;
                module.Description = string.Empty;
                module.Name = name;
                module.Version = string.Empty;
                module.Header = ReadScriptFile(enc, headerSrcFileName, "// " + name + " module header\r\n");
                module.Script = ReadScriptFile(enc, scriptSrcFileName, "// " + name + " module script\r\n");
                module.UniqueKey = uniqueKey;

                List<Script> newScripts = ImportExport.AddImportedScriptModule(module);

                AddScriptFromImportOrFiles(name, newScripts);
                return GetNodeID(module);
            }
            catch (Exception ex)
            {
                errors.Add(new CompileError("An error occurred adding scripts. " +  ex.ToString()));
            }
            return string.Empty;
        }

        // Helper to show error message boxes, using MessageBoxOnCompile config for now
        private void ShowMessageAsNeeded(CompileMessages errors, string message)
        {
            MessageBoxOnCompile msgBoxSetting = Factory.AGSEditor.Settings.MessageBoxOnCompile;
            bool isMsgBoxShownOnError = msgBoxSetting != MessageBoxOnCompile.Never;
            bool isMsgBoxShownOnWarning = isMsgBoxShownOnError && msgBoxSetting != MessageBoxOnCompile.OnlyErrors;

            Factory.GUIController.ShowOutputPanel(errors);
            if (errors.HasErrors && isMsgBoxShownOnError)
            {
                _guiController.ShowMessage(message, MessageBoxIcon.Error);
            }
            else if (isMsgBoxShownOnWarning)
            {
                _guiController.ShowMessage(message, MessageBoxIcon.Warning);
            }
        }

        private void AddScriptModules(string[] fileNames)
        {
            CompileMessages errors = new CompileMessages();

            List<ExistingScriptHeaderToAdd> unfixedcriptHeaderPairs = GetNormalizedScriptHeaderPairs(fileNames, errors);

            if (unfixedcriptHeaderPairs.Count == 0 && errors.Count > 0)
            {
                Factory.GUIController.ShowOutputPanel(errors);
                ShowMessageAsNeeded(errors, "Selected files could not be added, see output panel for more details");
                return;
            }

            List<ExistingScriptHeaderToAdd> uncheckedScriptHeaderPairs = FixDestinationFileNames(unfixedcriptHeaderPairs);
            List<ExistingScriptHeaderToAdd> scriptHeaderPairs = SafelyAddMissingPairIfExists(uncheckedScriptHeaderPairs);

            string lastAddedScriptNodeID = string.Empty;
            foreach (ExistingScriptHeaderToAdd pair in scriptHeaderPairs)
            {
                lastAddedScriptNodeID = AddExistingScriptFile(pair, errors);
            }

            Factory.GUIController.ClearOutputPanel();

            // if at least one script module is successfully added, this does two things
            // - it selects the last added module to make it easier to find it
            // - it refreshes the treeview, if we don't do this it appears the script was added at bottom, but any later refresh will move it.
            if (!string.IsNullOrEmpty(lastAddedScriptNodeID))
            {
                RePopulateTreeView(lastAddedScriptNodeID);
                ScriptListTypeConverter.SetScriptList(Factory.AGSEditor.CurrentGame.ScriptsAndHeaders);
            }

            if (errors.Count > 0)
            {
                Factory.GUIController.ShowOutputPanel(errors);
                ShowMessageAsNeeded(errors, "Some of the selected files could not be added, see output panel for more details");
            }
        }

        private void AddScriptFromImportOrFiles(string destFileName, List<Script> newScripts)
        {
            newScripts[0].FileName = destFileName + ".ash";
            newScripts[1].FileName = destFileName + ".asc";
            newScripts[0].Modified = true;
            newScripts[1].Modified = true;
            newScripts[0].SaveToDisk();
            newScripts[1].SaveToDisk();
            ScriptAndHeader scripts = new ScriptAndHeader(newScripts[0], newScripts[1]);
            AddSingleItem(scripts);
            ScriptListTypeConverter.SetScriptList(Factory.AGSEditor.CurrentGame.ScriptsAndHeaders);
            _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
            foreach (Script script in newScripts)
                AutoComplete.ConstructCache(script, _agsEditor.GetImportedScriptHeaders(script));
        }

        private void ImportScriptModule(string fileName)
        {
            try
            {
                string destFileName = FindFirstAvailableFileName(Path.GetFileNameWithoutExtension(fileName));
                List<Script> newScripts = ImportExport.ImportScriptModule(fileName, _agsEditor.CurrentGame.TextEncoding);
                AddScriptFromImportOrFiles(destFileName, newScripts);
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
            ContentDocument document = _editors[chosenItem];
            document.TreeNodeID = GetNodeID(chosenItem);
            _guiController.AddOrShowPane(document);
            if (activateEditor)
            {
                // Hideous hack -- we need to allow the current message to
                // finish processing before setting the focus to the
                // script window, or it will fail
                TickOnceTimer.CreateAndStart(10, new EventHandler((sender, e) => ActivateWindow_Tick(scriptEditor)));
            }
            return scriptEditor;
        }

        private void ActivateWindow_Tick(ScriptEditor scriptEditor)
        {
            scriptEditor.ActivateTextEditor();
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

            if (evArgs.Handled)
            {
                return; // operation has been completed by another handler
            }

            if (evArgs.FileName == Tasks.AUTO_GENERATED_HEADER_NAME)
			{
                evArgs.Result = ZoomToFileResult.ScriptNotFound;
                _guiController.ShowMessage("The error was within an automatically generated script file, so you cannot edit the script. The most likely cause of errors here is that two things in your game have the same name. For example, two characters or two fonts with the same script name could cause this error. Please consult the error message for more clues.", MessageBoxIcon.Warning);
				return;
			}

            ScriptEditor editor = CreateOrShowEditorForScript(evArgs.FileName, evArgs.ActivateEditor);
            if (editor != null)
            {
                ZoomToCorrectPositionInScript(editor, evArgs);
                return;
            }

            evArgs.Result = ZoomToFileResult.ScriptNotFound;
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

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(MENU_COMMAND_NEW, "New script", null));
            menu.Add(new MenuCommand(MENU_COMMAND_IMPORT, "Import script module...", null));
            menu.Add(new MenuCommand(MENU_COMMAND_ADD_EXISTING, "Add existing script...", null));
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
            Script.TextEncoding = _agsEditor.CurrentGame.TextEncoding;

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
            ScriptListTypeConverter.SetScriptList(Factory.AGSEditor.CurrentGame.ScriptsAndHeaders);
        }

        public override void GameSettingsChanged()
        {
            Script.TextEncoding = _agsEditor.CurrentGame.TextEncoding;
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
                _rightClickedScript = commandID.Substring(ITEM_COMMAND_PREFIX.Length) + ".ash";
                TickOnceTimer.CreateAndStart(10, new EventHandler((sender, e) => ScriptRenamed_Tick(_rightClickedScript, treeItem)));
            }
        }

        private void ScriptRenamed_Tick(string scriptName, ProjectTreeItem treeItem)
        {
            ScriptRenamed(scriptName, treeItem);
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

            if (!Validation.FilenameIsValid(renamedScript.FileName))
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
                _agsEditor.RenameFileOnDisk(oldScriptName, renamedScript.FileName);
				_agsEditor.RenameFileOnDisk(associatedScript.FileName, newNameForAssociatedScript);
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
            ScriptListTypeConverter.SetScriptList(Factory.AGSEditor.CurrentGame.ScriptsAndHeaders);
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

        private string GetNodeID(ScriptModuleDef scriptDef)
        {
            return ITEM_COMMAND_PREFIX + scriptDef.Name;
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

        protected override IList<ScriptAndHeader> GetFlatList()
        {
            return null;
        }

        private void Events_GamePostLoad(Game game)
        {
            if (game.SavedXmlVersionIndex >= 3060110)
                return; // no upgrade necessary

            // < 3060110 - SetRestartPoint() has to be added to Global Script's game_start,
            // emulate legacy behavior where its call was hardcoded in the engine.
            if (game.SavedXmlVersionIndex < 3060110)
            {
                Script script = game.RootScriptFolder.GetScriptByFileName(Script.GLOBAL_SCRIPT_FILE_NAME, true);
                if (script != null)
                {
                    script.Text = 
                        ScriptGeneration.InsertFunction(script.Text, "game_start", "", "  SetRestartPoint();", amendExisting: true);
                    // CHECKME: do not save the script here, in case user made a mistake opening this in a newer editor
                    // and closes project without saving after upgrade? Upgrade process is not well defined...
                }
            }
        }
    }
}
