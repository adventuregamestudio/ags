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
    class InventoryComponent : BaseComponentWithFolders<InventoryItem, InventoryItemFolder>
    {
        private const string INVENTORY_COMMAND_ID = "Inventory";
        private const string COMMAND_NEW_ITEM = "NewInventory";
        private const string COMMAND_DELETE_ITEM = "DeleteInventory";
        private const string COMMAND_FIND_ALL_USAGES = "FindAllUsages";
        private const string ICON_KEY = "InventorysIcon";
        
        private Dictionary<InventoryItem, ContentDocument> _documents;
        private InventoryItem _itemRightClicked = null;

        public InventoryComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, INVENTORY_COMMAND_ID)
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

        protected override void ItemCommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                if (_agsEditor.CurrentGame.InventoryItems.Count == Game.MAX_INV_ITEMS)
                {
                    Factory.GUIController.ShowMessage("You already have the maximum number of inventory items in your game, and cannot add any more.", MessageBoxIcon.Warning);
                    return;
                }
                InventoryItem newItem = new InventoryItem();
                newItem.ID = _agsEditor.CurrentGame.RootInventoryItemFolder.GetAllItemsCount() + 1;
                newItem.Name = _agsEditor.GetFirstAvailableScriptName("iInvItem");
                newItem.Description = "New inventory item";
                string newNodeID = AddSingleItem(newItem);
                _guiController.ProjectTree.SelectNode(this, newNodeID);                
				ShowOrAddPane(newItem);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this inventory item? Doing so could break any scripts that refer to inventory items by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    DeleteSingleItem(_itemRightClicked);                    
                }
            }
            else if (controlID == COMMAND_FIND_ALL_USAGES)
            {
                FindAllUsages findAllUsages = new FindAllUsages(null, null, null, _agsEditor);
                findAllUsages.Find(null, _itemRightClicked.Name);
            }
            else if (controlID != TOP_LEVEL_COMMAND_ID && !IsFolderNode(controlID))
            {
                InventoryItem chosenItem = _agsEditor.CurrentGame.RootInventoryItemFolder.FindInventoryItemByID(
                    Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length)), true);
                ShowOrAddPane(chosenItem);
            }
        }

        private void DeleteInventoryItem(InventoryItem inventoryItem)
        {
            int removingID = inventoryItem.ID;
            foreach (InventoryItem item in _agsEditor.CurrentGame.RootInventoryItemFolder.AllItemsFlat)
            {
                if (item.ID > removingID)
                {
                    item.ID--;
                }
            }
            ContentDocument document;
            if (_documents.TryGetValue(inventoryItem, out document))
            {
                _guiController.RemovePaneIfExists(document);
                _documents.Remove(inventoryItem);
            }
        }

        protected override void DeleteResourcesUsedByItem(InventoryItem item)
        {
            DeleteInventoryItem(item);
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
            document.TreeNodeID = GetNodeID(chosenItem);
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

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New Inventory Item", null));
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
                int invID = Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.RootInventoryItemFolder.FindInventoryItemByID(invID, true);
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this item", null));
                menu.Add(new MenuCommand(COMMAND_FIND_ALL_USAGES, "Find All Usages of " + _itemRightClicked.Name, null));
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

        protected override ProjectTreeItem CreateTreeItemForItem(InventoryItem item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item), item.ID.ToString() + ": " + item.Name, "InventoryIcon");
            return treeItem;
        }

        private string GetNodeID(InventoryItem item)
        {
            return ITEM_COMMAND_PREFIX + item.ID; 
        }
        
        private Dictionary<string, object> ConstructPropertyObjectList(InventoryItem item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.Name + " (Inventory item " + item.ID + ")", item);
            return list;
        }

        protected override bool CanFolderBeDeleted(InventoryItemFolder folder)
        {
            return true;
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its inventory items?" + Environment.NewLine + Environment.NewLine + "If any of the items are referenced in code by their number it could cause crashes in the game.";
        }

        protected override InventoryItemFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootInventoryItemFolder;
        }
    }
}
