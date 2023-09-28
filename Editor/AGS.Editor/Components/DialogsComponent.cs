using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;
using AGS.Editor.TextProcessing;

namespace AGS.Editor.Components
{
    class DialogsComponent : BaseComponentWithFolders<Dialog, DialogFolder>
    {
        private const string DIALOGS_COMMAND_ID = "Dialogs";
        private const string COMMAND_NEW_ITEM = "NewDialog";
        private const string COMMAND_DELETE_ITEM = "DeleteDialog";
        private const string COMMAND_CHANGE_ID = "ChangeDialogID";
        private const string COMMAND_FIND_ALL_USAGES = "FindAllUsages";
        private const string ICON_KEY = "DialogsIcon";
        
        private Dictionary<Dialog, ContentDocument> _documents;
        private Dialog _itemRightClicked = null;

        public DialogsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, DIALOGS_COMMAND_ID)
        {
            _documents = new Dictionary<Dialog, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("dialog.ico"));
            _guiController.RegisterIcon("DialogIcon", Resources.ResourceManager.GetIcon("dialog-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Dialogs", ICON_KEY);
			_guiController.OnZoomToFile += new GUIController.ZoomToFileHandler(GUIController_OnZoomToFile);
            _guiController.OnGetScriptEditorControl += new GUIController.GetScriptEditorControlHandler(_guiController_OnGetScriptEditorControl);
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

        protected override void ItemCommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                Dialog newItem = new Dialog();
                newItem.ID = _agsEditor.CurrentGame.RootDialogFolder.GetAllItemsCount();
                newItem.Name = _agsEditor.GetFirstAvailableScriptName("dDialog");
                string newNodeID = AddSingleItem(newItem);
                _guiController.ProjectTree.SelectNode(this, newNodeID);
				ShowPaneForDialog(newItem);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this dialog? Doing so will break any scripts that refer to dialogs by their number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    DeleteSingleItem(_itemRightClicked);
                }
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
                GetFlatList().Swap(oldNumber, newNumber);
                OnItemIDOrNameChanged(_itemRightClicked, false);
            }
            else if (controlID == COMMAND_FIND_ALL_USAGES)
            {
                FindAllUsages findAllUsages = new FindAllUsages(null, null, null, _agsEditor);
                findAllUsages.Find(null, _itemRightClicked.Name);
            }
            else if ((!controlID.StartsWith(NODE_ID_PREFIX_FOLDER)) &&
                     (controlID != TOP_LEVEL_COMMAND_ID))
            {
                ShowPaneForDialog(Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length)));
            }
        }

        private void DeleteDialog(Dialog dialog)
        {
            int removingID = dialog.ID;
            foreach (Dialog item in _agsEditor.CurrentGame.RootDialogFolder.AllItemsFlat)
            {
                if (item.ID > removingID)
                {
                    item.ID--;
                }
                // Force a refresh since ID's have changed
                item.CachedConvertedScript = null;
            }

            ContentDocument document;
            if (_documents.TryGetValue(dialog, out document))
            {
                _guiController.RemovePaneIfExists(document);
                _documents.Remove(dialog);
            }
        }

        protected override void DeleteResourcesUsedByItem(Dialog item)
        {
            DeleteDialog(item);
        }

        private void OnItemIDOrNameChanged(Dialog item, bool name_only)
        {
            // Refresh tree, property grid and open windows
            if (name_only)
                ChangeItemLabel(GetNodeID(item), GetNodeLabel(item));
            else
                RePopulateTreeView(); // currently this is the only way to update tree item ids

            foreach (ContentDocument doc in _documents.Values)
            {
                var docItem = ((DialogEditor)doc.Control).ItemToEdit;
                doc.Name = docItem.WindowTitle;
                _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(docItem), doc, docItem);
            }

            // Force re-build of dialog scripts since names/ids have changed
            foreach (Dialog dlg in _agsEditor.CurrentGame.RootDialogFolder.AllItemsFlat)
            {
                dlg.CachedConvertedScript = null;
            }
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            Dialog itemBeingEdited = ((DialogEditor)_guiController.ActivePane.Control).ItemToEdit;

            if (propertyName == "Name")
            {
                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.Name, itemBeingEdited))
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIcon.Warning);
                    itemBeingEdited.Name = (string)oldValue;
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
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            // No more commands in this menu
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            if ((controlID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!IsFolderNode(controlID)))            
            {
                int dialogID = Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.RootDialogFolder.FindDialogByID(dialogID, true);
                menu.Add(new MenuCommand(COMMAND_CHANGE_ID, "Change dialog ID", null));
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this dialog", null));
                menu.Add(new MenuCommand(COMMAND_FIND_ALL_USAGES, "Find All Usages of " + _itemRightClicked.Name, null));
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

        public override void ShowItemPaneByName(string name)
        {
            IList<Dialog> dialogs = GetFlatList();
            foreach (Dialog d in dialogs)
            {
                if (d.Name == name)
                {
                    _guiController.ProjectTree.SelectNode(this, GetNodeID(d));
                    ShowPaneForDialog(d);
                    return;
                }
            }
        }

        private DialogEditor ShowPaneForDialog(int dialogNumber)
		{
            Dialog chosenItem = _agsEditor.CurrentGame.RootDialogFolder.FindDialogByID(dialogNumber, true);
            return ShowPaneForDialog(chosenItem);
		}

        private DialogEditor ShowPaneForDialog(Dialog chosenItem)
        {
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
            Dialog dialog = _agsEditor.CurrentGame.RootDialogFolder.FindDialogByID(dialogNumber, true);

            return dialog;	
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
            }         
		}

        private string GetNodeID(Dialog item)
        {
            return ITEM_COMMAND_PREFIX + item.ID;
        }

        private string GetNodeLabel(Dialog item)
        {
            return item.ID.ToString() + ": " + item.Name;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(Dialog item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item), GetNodeLabel(item), "DialogIcon");
            return treeItem;
        }
        
        private Dictionary<string, object> ConstructPropertyObjectList(Dialog item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.Name + " (Dialog " + item.ID + ")", item);
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

        protected override IList<Dialog> GetFlatList()
        {
            return _agsEditor.CurrentGame.DialogFlatList;
        }
    }
}
