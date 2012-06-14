using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class InventoryComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Inventory";
        private const string COMMAND_NEW_ITEM = "NewInventory";
        private const string COMMAND_DELETE_ITEM = "DeleteInventory";
        private const string ICON_KEY = "InventorysIcon";
        
        private Dictionary<InventoryItem, ContentDocument> _documents;
        private InventoryItem _itemRightClicked = null;

        public InventoryComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _documents = new Dictionary<InventoryItem, ContentDocument>();
            _guiController.RegisterIcon("InventoryIcon", Resources.ResourceManager.GetIcon("iconinv-item.ico"));
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("iconinv.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Inventory items", ICON_KEY);
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Inventory; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                IList<InventoryItem> items = _agsEditor.CurrentGame.InventoryItems;
                InventoryItem newItem = new InventoryItem();
                newItem.ID = items.Count + 1;
                newItem.Name = _agsEditor.GetFirstAvailableScriptName("iInvItem");
                newItem.Description = "New inventory item";
                items.Add(newItem);
                _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
                _guiController.ProjectTree.AddTreeLeaf(this, "Inv" + newItem.ID, newItem.ID.ToString() + ": " + newItem.Name, "InventoryIcon");
                _guiController.ProjectTree.SelectNode(this, "Inv" + newItem.ID);
				ShowOrAddPane(newItem);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this inventory item? Doing so could break any scripts that refer to inventory items by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    int removingID = _itemRightClicked.ID;
                    foreach (InventoryItem item in _agsEditor.CurrentGame.InventoryItems)
                    {
                        if (item.ID > removingID)
                        {
                            item.ID--;
                        }
                    }
                    if (_documents.ContainsKey(_itemRightClicked))
                    {
                        _guiController.RemovePaneIfExists(_documents[_itemRightClicked]);
                        _documents.Remove(_itemRightClicked);
                    }
                    _agsEditor.CurrentGame.InventoryItems.Remove(_itemRightClicked);
                    RePopulateTreeView();
                }
            }
            else if (controlID != TOP_LEVEL_COMMAND_ID)
            {
                InventoryItem chosenItem = _agsEditor.CurrentGame.InventoryItems[Convert.ToInt32(controlID.Substring(3)) - 1];
				ShowOrAddPane(chosenItem);
			}
        }

		private void ShowOrAddPane(InventoryItem chosenItem)
		{
            ContentDocument document;
			if (!_documents.TryGetValue(chosenItem, out document)
                || document.Control.IsDisposed)
			{
                document = new ContentDocument(new InventoryEditor(chosenItem),
                    chosenItem.WindowTitle, this, ICON_KEY,
                    ConstructPropertyObjectList(chosenItem));
				_documents[chosenItem] = document;
                document.SelectedPropertyGridObject = chosenItem;
			}
            _guiController.AddOrShowPane(document);
			_guiController.ShowCuppit("Inventory items are things that characters can carry around with them. You can set up this inventory item using the property grid on the right.", "Inventory introduction");
		}

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == "Name")
            {
                InventoryItem itemToChange = ((InventoryEditor)_guiController.ActivePane.Control).ItemToEdit;

                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemToChange.Name, itemToChange))
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIcon.Warning);
                    itemToChange.Name = (string)oldValue;
                }
                else
                {
                    RePopulateTreeView();
                    _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(itemToChange));

                    foreach (ContentDocument doc in _documents.Values)
                    {
                        doc.Name = ((InventoryEditor)doc.Control).ItemToEdit.WindowTitle;
                    }
                }
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New Inventory Item", null));
            }
            else
            {
                int invID = Convert.ToInt32(controlID.Substring(3));
                _itemRightClicked = _agsEditor.CurrentGame.InventoryItems[invID - 1];
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this item", null));
            }
            return menu;
        }

        public override void BeforeSaveGame()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                ((InventoryEditor)doc.Control).SaveData();
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

        private void RePopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            foreach (InventoryItem item in _agsEditor.CurrentGame.InventoryItems)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, "Inv" + item.ID, item.ID.ToString() + ": " + item.Name, "InventoryIcon");
            }

            if (_documents.ContainsValue(_guiController.ActivePane))
            {
                InventoryEditor editor = (InventoryEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, "Inv" + editor.ItemToEdit.ID);
            }
            else if (_agsEditor.CurrentGame.InventoryItems.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "Inv1");
            }
        }

        private Dictionary<string, object> ConstructPropertyObjectList(InventoryItem item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.Name + " (Inventory item " + item.ID + ")", item);
            return list;
        }
    }
}
