using AGS.Editor.TextProcessing;
using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using System.Xml;

namespace AGS.Editor.Components
{
    class DialogsComponent : BaseComponentWithFolders<DialogRef, DialogFolder>
    {
        private const string DIALOGS_COMMAND_ID = "Dialogs";
        private const string COMMAND_NEW_ITEM = "NewDialog";
        private const string COMMAND_DELETE_ITEM = "DeleteDialog";
        private const string COMMAND_COPY_ITEM = "CopyDialog";
        private const string COMMAND_PASTE_ITEM = "PasteDialog";
        private const string COMMAND_CHANGE_ID = "ChangeDialogID";
        private const string COMMAND_FIND_ALL_USAGES = "FindAllUsages";
        private const string COMMAND_GO_TO_DIALOG_NUMBER = "GoToDialogNumber";
        private const string ICON_KEY = "DialogsIcon";
        
        private Dictionary<Dialog, ContentDocument> _documents;
        private DialogRef _itemRightClicked = null;
        // Session-long dialog GUIDs, used to uniquely identify dialog's instance.
        // It may make sense to create a Guid field in the Dialog itself (and other game types),
        // but i am keeping this here until that is decided.
        private Dictionary<Dialog, Guid> _dialogGuids = new Dictionary<Dialog, Guid>();
        // Map of the files assigned to the dialogs, used to track changes when removing or renaming dialogs.
        private struct DialogFiles
        {
            public string xmlFile;
            public string scriptFile;
            public DialogFiles(string xmlfile, string scriptfile)
            {
                xmlFile = xmlfile;
                scriptFile = scriptfile;
            }
        }
        private Dictionary<Guid, DialogFiles> _dialogFiles = new Dictionary<Guid, DialogFiles>();

        public DialogsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, DIALOGS_COMMAND_ID)
        {
            _documents = new Dictionary<Dialog, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("dialog.ico"));
            _guiController.RegisterIcon("DialogIcon", Resources.ResourceManager.GetIcon("dialog-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Dialogs", ICON_KEY);
			_guiController.OnZoomToFile += GUIController_OnZoomToFile;
            _guiController.OnGetScriptEditorControl += _guiController_OnGetScriptEditorControl;
            Factory.Events.GamePostLoad += Events_GamePostLoad;
            Factory.Events.GamePostSave += Events_GamePostSave;
            RePopulateTreeView();
        }

        private void _guiController_OnGetScriptEditorControl(GetScriptEditorControlEventArgs evArgs)
        {
            Dialog chosenItem = GetDialog(evArgs.ScriptFileName);
            if (chosenItem != null)
            {
                AddDocumentIfNeeded(evArgs.ShowEditor, chosenItem);

                evArgs.ScriptEditor = ((DialogEditor)_documents[chosenItem].Control).ScriptEditor;
            }
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Dialogs; }
        }

        private void Events_GamePostLoad(Game game)
        {
            _dialogGuids.Clear();
            _dialogFiles.Clear();
            // After the game document is loaded, we load all dialog documents,
            // filling in actual dialog data. The scripts are not loaded here though,
            // they are loaded only when the dialog editor is about to open.
            foreach (Dialog dialog in game.Dialogs)
            {
                LoadDialogFromXml(dialog);

                _dialogGuids[dialog] = Guid.NewGuid();
                _dialogFiles[_dialogGuids[dialog]] = new DialogFiles(dialog.DataFileName, dialog.ScriptFileName);
            }
        }

        private void Events_GamePostSave(Game game)
        {
            if (!Directory.Exists(Dialog.DIALOG_FILES_DIRECTORY))
            {
                Directory.CreateDirectory(Dialog.DIALOG_FILES_DIRECTORY);
            }

            // Try to make things a bit safer (in case Editor crashing in the process, for example);
            // keep obsolete dialog files on disk until all dialogs are saved,
            // and then delete only those that were not resaved by a new dialog of the same name.
            Dictionary<Guid, DialogFiles> oldDialogFiles = _dialogFiles;
            HashSet<string> oldFilenames = new HashSet<string>();
            foreach (var files in _dialogFiles)
            {
                oldFilenames.Add(files.Value.xmlFile);
                oldFilenames.Add(files.Value.scriptFile);
            }

            _dialogFiles = new Dictionary<Guid, DialogFiles>();
            // Write dialogs into their respective documents
            foreach (Dialog dialog in game.Dialogs)
            {
                // TODO: consider adding "modified" flag to the Dialog itself, and check here.
                SaveDialogToXml(dialog);
                // Force script to save if the filename was changed
                bool mustSave =
                    !oldDialogFiles.ContainsKey(_dialogGuids[dialog]) || (oldDialogFiles[_dialogGuids[dialog]].scriptFile != dialog.Script.FileName);
                dialog.Script.SaveToDisk(mustSave);

                _dialogFiles[_dialogGuids[dialog]] = new DialogFiles(dialog.DataFileName, dialog.ScriptFileName);
            }

            // Filter out and remove dialog xmls that are no longer attached to existing dialogs
            foreach (var files in _dialogFiles)
            {
                oldFilenames.Remove(files.Value.xmlFile);
                oldFilenames.Remove(files.Value.scriptFile);
            }
            foreach (var file in oldFilenames)
            {
                Utilities.TryDeleteFile(file);
            }
        }

        /// <summary>
        /// Loads dialog data from its xml document.
        /// </summary>
        private void LoadDialogFromXml(Dialog dialog)
        {
            XmlDocument xml = Utilities.LoadXml(dialog.DataFileName);
            if (xml != null)
            {
                dialog.LoadFromXml(xml.SelectSingleNode("Dialog"));
            }
        }

        /// <summary>
        /// Saves dialog data to its xml document.
        /// </summary>
        private void SaveDialogToXml(Dialog dialog)
        {
            using (var writer = new XmlTextWriter(dialog.DataFileName, Types.Utilities.UTF8))
            {
                writer.Formatting = Formatting.Indented;
                dialog.ToXmlDocument().Save(writer);
            }
        }

        private Dialog AddNewDialog(Dialog newDialog, string baseScriptName)
        {
            newDialog.ID = _agsEditor.CurrentGame.RootDialogFolder.GetAllItemsCount();
            newDialog.ScriptName = _agsEditor.GetFirstAvailableScriptName(baseScriptName);
            // NOTE: this assumes that Dialogs list always stays sorted and without gaps
            _agsEditor.CurrentGame.Dialogs.Add(newDialog);
            _dialogGuids[newDialog] = Guid.NewGuid();
            string newNodeID;
            if (_itemRightClicked != null)
                newNodeID = AddSingleItem(new DialogRef(newDialog), GetNodeIDForFolder(FindFolderThatContainsItem(GetRootFolder(), _itemRightClicked)));
            else
                newNodeID = AddSingleItem(new DialogRef(newDialog), _rightClickedID);
            _guiController.ProjectTree.SelectNode(this, newNodeID);
            ShowPaneForDialog(newDialog);
            return newDialog;
        }

        protected override void ItemCommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                Dialog newItem = new Dialog();
                AddNewDialog(newItem, "dDialog");
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this dialog? Doing so will break any scripts that refer to dialogs by their number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    DeleteSingleItem(_itemRightClicked);
                }
            }
            else if (controlID == COMMAND_COPY_ITEM)
            {
                ClipboardUtils.CopyToClipboard(_itemRightClicked.Dialog);
            }
            else if (controlID == COMMAND_PASTE_ITEM)
            {
                Dialog newItem = ClipboardUtils.PasteFromClipboard(typeof(Dialog)) as Dialog;
                if (newItem == null)
                    return;
                AddNewDialog(newItem, newItem.ScriptName);
            }
            else if (controlID == COMMAND_CHANGE_ID)
            {
                int oldNumber = _itemRightClicked.ID;
                int newNumber = Factory.GUIController.ShowChangeObjectIDDialog("Dialog", oldNumber, 0, _items.Count - 1);
                if (newNumber < 0)
                    return;
                foreach (var obj in _items)
                {
                    if (obj.Value.ID == newNumber)
                    {
                        obj.Value.ID = oldNumber;
                        break;
                    }
                }
                _itemRightClicked.ID = newNumber;
                // NOTE: remember we must swap items in both Dialogs and DialogRefs list!
                _agsEditor.CurrentGame.Dialogs.Swap(oldNumber, newNumber);
                GetFlatList().Swap(oldNumber, newNumber);
                OnItemIDOrNameChanged(_itemRightClicked.Dialog, false);
            }
            else if (controlID == COMMAND_FIND_ALL_USAGES)
            {
                FindAllUsages findAllUsages = new FindAllUsages(null, null, null, _agsEditor);
                findAllUsages.Find(null, _itemRightClicked.ScriptName);
            }
            else if (controlID == COMMAND_GO_TO_DIALOG_NUMBER)
            {
                ShowGoToDialogDialog();
            }
            else if ((!controlID.StartsWith(NODE_ID_PREFIX_FOLDER)) &&
                     (controlID != TOP_LEVEL_COMMAND_ID))
            {
                ShowPaneForDialog(Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length)));
            }
        }

        private void ShowGoToDialogDialog()
        {
            IList<Types.Dialog> dialogs = Factory.AGSEditor.CurrentGame.Dialogs;
            if (dialogs.Count == 0) return;

            GoToNumberDialog goToDialogDialog = new GoToNumberDialog()
            {
                Text = "Go To Dialog",
                NodeTypeName = "Dialog",
                List = dialogs
                    .Select(d => Tuple.Create(d.ID, d.ScriptName))
                    .ToList()
            };
            if (goToDialogDialog.ShowDialog() != System.Windows.Forms.DialogResult.OK) return;

            int dialogNumber = goToDialogDialog.Number;
            Types.Dialog dialog = dialogs.Where(i => i.ID == dialogNumber).First();
            _guiController.ProjectTree.SelectNode(this, GetNodeID(dialog));
            ShowPaneForDialog(dialogNumber);
        }

        private void DeleteDialog(Dialog dialog)
        {
            _agsEditor.CurrentGame.Dialogs.Remove(dialog);
            // Must shift all greater IDs down to fill the gap;
            // do this both for DialogRef and Dialog
            foreach (DialogRef item in _agsEditor.CurrentGame.RootDialogFolder.AllItemsFlat)
            {
                if (item.ID > dialog.ID)
                {
                    item.ID--; // this also adjusts linked Dialog's ID
                    // Force a refresh since ID's have changed
                    item.Dialog.CachedConvertedScript = null;
                }
            }

            ContentDocument document;
            if (_documents.TryGetValue(dialog, out document))
            {
                _guiController.RemovePaneIfExists(document);
                _documents.Remove(dialog);
            }
        }

        protected override void DeleteResourcesUsedByItem(DialogRef item)
        {
            // Remove dialog from the guids, this marks associated xml document as obsolete
            _dialogGuids.Remove(item.Dialog);

            // Delete item itself
            DeleteDialog(item.Dialog);
        }

        private void OnItemIDOrNameChanged(Dialog item, bool name_only)
        {
            // If it was ID, then DialogRef was updated first, so sync only name here
            var dialogRef = _items.First((KeyValuePair<string, DialogRef> itemRef) => { return itemRef.Value.Dialog == item; });
            dialogRef.Value.ScriptName = item.ScriptName;

            // Refresh tree, property grid and open windows
            if (name_only)
                ChangeItemLabel(GetNodeID(item), GetNodeLabel(item));
            else
                RePopulateTreeView(GetNodeID(item)); // currently this is the only way to update tree item ids

            foreach (ContentDocument doc in _documents.Values)
            {
                var docItem = ((DialogEditor)doc.Control).ItemToEdit;
                doc.Name = docItem.WindowTitle;
                _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(docItem), doc, docItem);
            }

            // Force re-build of dialog scripts since names/ids have changed
            foreach (Dialog dlg in _agsEditor.CurrentGame.Dialogs)
            {
                dlg.CachedConvertedScript = null;
            }
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            Dialog itemBeingEdited = ((DialogEditor)_guiController.ActivePane.Control).ItemToEdit;

            if (propertyName == "ScriptName")
            {
                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.ScriptName, itemBeingEdited))
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIcon.Warning);
                    itemBeingEdited.ScriptName = (string)oldValue;
                }
                else
                {
                    OnItemIDOrNameChanged(itemBeingEdited, true);
                }
            }
        }

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New Dialog", null));
            if (ClipboardUtils.IsAvailableOnClipboard(typeof(Dialog)))
                menu.Add(new MenuCommand(COMMAND_PASTE_ITEM, "Paste Dialog", null));
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(MenuCommand.Separator);
                MenuCommand goToCommand = new MenuCommand(COMMAND_GO_TO_DIALOG_NUMBER, "Go to Dialog...", Keys.Control | Keys.G);
                goToCommand.Enabled = Factory.AGSEditor.CurrentGame.Dialogs.Count > 0;
                menu.Add(goToCommand);
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            _itemRightClicked = null;
            if ((controlID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!IsFolderNode(controlID)))            
            {
                int dialogID = Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.RootDialogFolder.FindDialogByID(dialogID, true);
                if (_itemRightClicked != null)
                {
                    menu.Add(new MenuCommand(COMMAND_CHANGE_ID, "Change dialog ID", null));
                    menu.Add(new MenuCommand(COMMAND_COPY_ITEM, "Copy dialog", null));
                    if (ClipboardUtils.IsAvailableOnClipboard(typeof(Dialog)))
                        menu.Add(new MenuCommand(COMMAND_PASTE_ITEM, "Paste dialog", null));
                    menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this dialog", null));
                    menu.Add(new MenuCommand(COMMAND_FIND_ALL_USAGES, "Find All Usages of " + _itemRightClicked.ScriptName, null));
                }
            }
            return menu;
        }

        public override void BeforeSaveGame()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                ((DialogEditor)doc.Control).SaveData();
            }
        }

        public override void RefreshDataFromGame()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                _guiController.RemovePaneIfExists(doc);
                doc.Dispose();
            }
            _documents.Clear();

            RePopulateTreeView();
        }

        private void AddDocumentIfNeeded(bool showEditor, Dialog chosenItem)
        {
            ContentDocument document;
            if (!_documents.TryGetValue(chosenItem, out document)
                || document.Control.IsDisposed)
            {
                DialogEditor dialogEditor = new DialogEditor(chosenItem, _agsEditor);
                dialogEditor.DockingContainer = new DockingContainer(dialogEditor);
                document = new ContentDocument(dialogEditor, chosenItem.WindowTitle,
                    this, ICON_KEY, ConstructPropertyObjectList(chosenItem));
                _documents[chosenItem] = document;
                document.SelectedPropertyGridObject = chosenItem;
                document.MainMenu = dialogEditor.ExtraMenu;
            }
            if (showEditor)
            {
                document.TreeNodeID = GetNodeID(chosenItem);
                _guiController.AddOrShowPane(document);
            }
        }

        public override IList<string> GetManagedScriptElements()
        {
            return new string[] { "Dialog" };
        }

        public override bool ShowItemPaneByName(string name)
        {
            IList<Dialog> dialogs = _agsEditor.CurrentGame.Dialogs;
            foreach (Dialog d in dialogs)
            {
                if (d.ScriptName == name)
                {
                    _guiController.ProjectTree.SelectNode(this, GetNodeID(d));
                    ShowPaneForDialog(d);
                    return true;
                }
            }
            return false;
        }

        private DialogEditor ShowPaneForDialog(int dialogNumber)
		{
            DialogRef chosenItem = _agsEditor.CurrentGame.RootDialogFolder.FindDialogByID(dialogNumber, true);
            return ShowPaneForDialog(chosenItem.Dialog);
		}

        private DialogEditor ShowPaneForDialog(Dialog chosenItem)
        {
            if (!chosenItem.Script.Modified && File.Exists(chosenItem.Script.FileName))
            {
                chosenItem.Script.LoadFromDisk();
            }

            AddDocumentIfNeeded(true, chosenItem);
            return (DialogEditor)_documents[chosenItem].Control;
        }

        private int GetDialogNumber(string name)
        {
            const string dialogPrefix = "Dialog ";
            if (name.StartsWith(dialogPrefix))
            {
                int num;
                return Int32.TryParse(name.Substring(dialogPrefix.Length), out num) ?
                    num : -1;
            }
            return -1;
        }

        private Dialog GetDialog(string name)
        {
            int dialogNumber = GetDialogNumber(name);
            if (dialogNumber < 0) return null;
            DialogRef dialogRef = _agsEditor.CurrentGame.RootDialogFolder.FindDialogByID(dialogNumber, true);
            return dialogRef?.Dialog;
        }

        private void RemoveExecutionPointFromAllScripts()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                ((DialogEditor)doc.Control).RemoveExecutionPointMarker();
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

            Dialog dialog = GetDialog(evArgs.FileName);
            if (dialog != null)
            {
                DialogEditor dialogEditor = ShowPaneForDialog(dialog);

                // has dialogEditor has already taken the area available?
                if (dialogEditor.Parent.ClientSize == dialogEditor.Size)
                {
                    // the Dialog Editor be already on screen!
                    dialogEditor.GoToScriptLine(evArgs);
                } 
                else
                {
                    // GoToScriptLine uses the size of scintilla control to calculate what lines are visibile and scroll accordingly
                    // since this is not the case, we will advance once the paint events happens, which is after layout adjustment
                    PaintEventHandler paintEvent = null;
                    paintEvent = (s, e1) =>
                    {
                        dialogEditor.GoToScriptLine(evArgs);
                        dialogEditor.Paint -= paintEvent;
                    };
                    dialogEditor.Paint += paintEvent;
                    dialogEditor.Invalidate();
                }
                evArgs.Result = ZoomToFileResult.Success;
            }
            else
            {
                evArgs.Result = ZoomToFileResult.ScriptNotFound;
            }
		}

        private string GetNodeID(Dialog item)
        {
            return ITEM_COMMAND_PREFIX + item.ID;
        }

        private string GetNodeLabel(Dialog item)
        {
            return item.ID.ToString() + ": " + item.ScriptName;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(DialogRef item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item.Dialog), GetNodeLabel(item.Dialog), "DialogIcon");
            return treeItem;
        }
        
        private Dictionary<string, object> ConstructPropertyObjectList(Dialog item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.PropertyGridTitle, item);
            return list;
        }

        protected override bool CanFolderBeDeleted(DialogFolder folder)
        {
            return true;
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its dialogs?" + Environment.NewLine + Environment.NewLine + "If any of the dialogs are referenced in code by their number it could cause crashes in the game.";
        }

        protected override DialogFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootDialogFolder;
        }

        protected override IList<DialogRef> GetFlatList()
        {
            return _agsEditor.CurrentGame.DialogFlatList;
        }
    }
}
