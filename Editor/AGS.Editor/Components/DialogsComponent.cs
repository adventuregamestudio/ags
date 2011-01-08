using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class DialogsComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Dialogs";
        private const string COMMAND_NEW_ITEM = "NewDialog";
        private const string COMMAND_DELETE_ITEM = "DeleteDialog";

        private Dictionary<Dialog, ContentDocument> _documents;
        private Dialog _itemRightClicked = null;

        public DialogsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _documents = new Dictionary<Dialog, ContentDocument>();
            _guiController.RegisterIcon("DialogsIcon", Resources.ResourceManager.GetIcon("dialog.ico"));
            _guiController.RegisterIcon("DialogIcon", Resources.ResourceManager.GetIcon("dialog-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Dialogs", "DialogsIcon");
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

        public override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                IList<Dialog> items = _agsEditor.CurrentGame.Dialogs;
                Dialog newItem = new Dialog();
                newItem.ID = items.Count;
                newItem.Name = _agsEditor.GetFirstAvailableScriptName("dDialog");
                items.Add(newItem);
                _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
                _guiController.ProjectTree.AddTreeLeaf(this, "Dlg" + newItem.ID, newItem.ID.ToString() + ": " + newItem.Name, "DialogIcon");
                _guiController.ProjectTree.SelectNode(this, "Dlg" + newItem.ID);
				ShowPaneForDialog(newItem);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this dialog? Doing so will break any scripts that refer to dialogs by their number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    int removingID = _itemRightClicked.ID;
                    foreach (Dialog item in _agsEditor.CurrentGame.Dialogs)
                    {
                        if (item.ID > removingID)
                        {
                            item.ID--;
                        }
                        // Force a refresh since ID's have changed
                        item.CachedConvertedScript = null;
                    }
                    if (_documents.ContainsKey(_itemRightClicked))
                    {
                        _guiController.RemovePaneIfExists(_documents[_itemRightClicked]);
                        _documents.Remove(_itemRightClicked);
                    }
                    _agsEditor.CurrentGame.Dialogs.Remove(_itemRightClicked);
                    RePopulateTreeView();
                }
            }
            else if (controlID != TOP_LEVEL_COMMAND_ID)
            {
				ShowPaneForDialog(Convert.ToInt32(controlID.Substring(3)));
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
                    // Refresh tree, property grid and open windows
                    RePopulateTreeView();
                    _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(itemBeingEdited));

                    foreach (ContentDocument doc in _documents.Values)
                    {
                        doc.Name = ((DialogEditor)doc.Control).ItemToEdit.WindowTitle;
                    }

                    // Force re-build of dialog scripts since names have changed
                    foreach (Dialog item in _agsEditor.CurrentGame.Dialogs)
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

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New Dialog", null));
            }
            else
            {
                int charID = Convert.ToInt32(controlID.Substring(3));
                _itemRightClicked = _agsEditor.CurrentGame.Dialogs[charID];
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this dialog", null));
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
            if (!_documents.ContainsKey(chosenItem))
            {
                DialogEditor dialogEditor = new DialogEditor(chosenItem, _agsEditor);
                _documents.Add(chosenItem, new ContentDocument(dialogEditor, chosenItem.WindowTitle, this, ConstructPropertyObjectList(chosenItem)));
                _documents[chosenItem].SelectedPropertyGridObject = chosenItem;
                _documents[chosenItem].MainMenu = dialogEditor.ExtraMenu;
            }
            if (showEditor)
            {
                _guiController.AddOrShowPane(_documents[chosenItem]);
            }
        }

		private DialogEditor ShowPaneForDialog(int dialogNumber)
		{
			Dialog chosenItem = _agsEditor.CurrentGame.Dialogs[dialogNumber];
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
            if (name.StartsWith("Dialog "))
            {
                return Convert.ToInt32(name.Substring(7));
            }
            return -1;
        }

        private Dialog GetDialog(string name)
        {
            int dialogNumber = GetDialogNumber(name);
            if (dialogNumber < 0) return null;
            Dialog dialog = _agsEditor.CurrentGame.Dialogs[dialogNumber];
            
            return _agsEditor.CurrentGame.Dialogs[dialogNumber];	
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

        private void RePopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            foreach (Dialog item in _agsEditor.CurrentGame.Dialogs)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, "Dlg" + item.ID, item.ID.ToString() + ": " + item.Name, "DialogIcon");
            }

            if (_documents.ContainsValue(_guiController.ActivePane))
            {
                DialogEditor editor = (DialogEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, "Dlg" + editor.ItemToEdit.ID);
            }
            else if (_agsEditor.CurrentGame.Dialogs.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "Dlg0");
            }
        }

        private Dictionary<string, object> ConstructPropertyObjectList(Dialog item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.Name + " (Dialog " + item.ID + ")", item);
            return list;
        }
    }
}
