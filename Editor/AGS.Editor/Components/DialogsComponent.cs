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
                    // Refresh tree, property grid and open windows
                    RePopulateTreeView();
                    _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(itemBeingEdited));

                    foreach (ContentDocument doc in _documents.Values)
                    {
                        doc.Name = ((DialogEditor)doc.Control).ItemToEdit.WindowTitle;
                    }

                    // Force re-build of dialog scripts since names have changed
                    foreach (Dialog item in _agsEditor.CurrentGame.RootDialogFolder.AllItemsFlat)
                    {
                        item.CachedConvertedScript = null;
                    }
                }
            }
            if (propertyName == "UniformMovementSpeed")
            {
                // Force the property grid to refresh and adjust which
                // properties are visible
                _guiController.SetPropertyGridObject(itemBeingEdited);
            }
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New Dialog", null));
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            if ((controlID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!IsFolderNode(controlID)))            
            {
                int dialogID = Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.RootDialogFolder.FindDialogByID(dialogID, true);
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

		private DialogEditor ShowPaneForDialog(int dialogNumber)
		{
            Dialog chosenItem = _agsEditor.CurrentGame.RootDialogFolder.FindDialogByID(dialogNumber, true);
            return ShowPaneForDialog(chosenItem);
		}

        private DialogEditor ShowPaneForDialog(Dialog chosenItem)
        {
            AddDocumentIfNeeded(true, chosenItem);
            _guiController.ShowCuppit("The Dialog Editor is where you set up conversations that the player can have with other characters. The possible options for this dialog topic are on the left, and the script that will run when the player chooses one is on the right.", "Dialog introduction");

            return (DialogEditor)_documents[chosenItem].Control;
        }

        private int GetDialogNumber(string name)
        {
            const string dialogPrefix = "Dialog ";
            if (name.StartsWith(dialogPrefix))
            {
                return Convert.ToInt32(name.Substring(dialogPrefix.Length));
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
                ShowPaneForDialog(dialog).GoToScriptLine(evArgs);
            }         
		}

        private string GetNodeID(Dialog item)
        {
            return ITEM_COMMAND_PREFIX + item.ID;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(Dialog item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item), item.ID.ToString() + ": " + item.Name, "DialogIcon");
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
    }
}
