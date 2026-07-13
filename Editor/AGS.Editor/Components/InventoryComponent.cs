using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
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
        private const string COMMAND_COPY_ITEM = "CopyInventory";
        private const string COMMAND_PASTE_ITEM = "PasteInventory";
        private const string COMMAND_CHANGE_ID = "ChangeInventoryID";
        private const string COMMAND_FIND_ALL_USAGES = "FindAllUsages";
        private const string COMMAND_GO_TO_ITEM_NUMBER = "GoToItemNumber";
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
            _agsEditor.CheckGameScripts += ScanAndReportMissingEventHandlers;
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Inventory; }
        }

        private InventoryItem AddNewItem(InventoryItem newItem, string baseScriptName)
        {
            newItem.ID = _agsEditor.CurrentGame.RootInventoryItemFolder.GetAllItemsCount() + 1;
            newItem.ScriptName = _agsEditor.GetFirstAvailableScriptName(baseScriptName);
            string newNodeID;
            if (_itemRightClicked != null)
                newNodeID = AddSingleItem(newItem, GetNodeIDForFolder(FindFolderThatContainsItem(GetRootFolder(), _itemRightClicked)));
            else
                newNodeID = AddSingleItem(newItem, _rightClickedID);
            _guiController.ProjectTree.SelectNode(this, newNodeID);
            ShowOrAddPane(newItem);
            return newItem;
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
                AddNewItem(newItem, "iInvItem");
                newItem.DisplayName = "New inventory item";
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this inventory item? Doing so could break any scripts that refer to inventory items by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    DeleteSingleItem(_itemRightClicked);                    
                }
            }
            else if (controlID == COMMAND_COPY_ITEM)
            {
                ClipboardUtils.CopyToClipboard(_itemRightClicked);
            }
            else if (controlID == COMMAND_PASTE_ITEM)
            {
                InventoryItem newItem = ClipboardUtils.PasteFromClipboard(typeof(InventoryItem)) as InventoryItem;
                if (newItem == null)
                    return;
                newItem.Interactions.Schema = InteractionSchema.Instance; // schema reference is lost when deserializing
                AddNewItem(newItem, newItem.ScriptName);
            }
            else if (controlID == COMMAND_CHANGE_ID)
            {
                int oldNumber = _itemRightClicked.ID;
                // NOTE: inventory item IDs are 1-based
                int newNumber = Factory.GUIController.ShowChangeObjectIDDialog("Inventory", oldNumber, 1, _items.Count);
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
                GetFlatList().Swap(oldNumber - 1, newNumber - 1); // inventory IDs are logically 1-based
                OnItemIDOrNameChanged(_itemRightClicked, false);
            }
            else if (controlID == COMMAND_FIND_ALL_USAGES)
            {
                FindAllUsages findAllUsages = new FindAllUsages(null, null, null, _agsEditor);
                findAllUsages.Find(null, _itemRightClicked.ScriptName);
            }
            else if (controlID == COMMAND_GO_TO_ITEM_NUMBER)
            {
                ShowGoToItemDialog();
            }
            else if (controlID != TOP_LEVEL_COMMAND_ID && !IsFolderNode(controlID))
            {
                InventoryItem chosenItem = _agsEditor.CurrentGame.RootInventoryItemFolder.FindInventoryItemByID(
                    Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length)), true);
                ShowOrAddPane(chosenItem);
            }
        }

        private void ShowGoToItemDialog()
        {
            IList<InventoryItem> items = GetFlatList();
            if (items.Count == 0) return;

            GoToNumberDialog goToItemDialog = new GoToNumberDialog()
            {
                Text = "Go To Inventory Item",
                NodeTypeName = "Inventory Item",
                List = items
                    .Select(i => Tuple.Create(i.ID, string.Format("({0}) {1}", i.ScriptName, i.DisplayName)))
                    .ToList()
            };
            if (goToItemDialog.ShowDialog() != DialogResult.OK) return;

            int itemNumber = goToItemDialog.Number;
            InventoryItem item = items.Where(i => i.ID == itemNumber).First();
            _guiController.ProjectTree.SelectNode(this, GetNodeID(item));
            ShowOrAddPane(item);
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
		}

        public override IList<string> GetManagedScriptElements()
        {
            return new string[] { "InventoryItem" };
        }

        public override bool ShowItemPaneByName(string name)
        {
            InventoryItem selectedItem = _agsEditor.CurrentGame.RootInventoryItemFolder.FindInventoryItemByName(name, true);
            if (selectedItem == null)
                return false;
            _guiController.ProjectTree.SelectNode(this, GetNodeID(selectedItem));
            ShowOrAddPane(selectedItem);
            return true;
        }

        private void OnItemIDOrNameChanged(InventoryItem item, bool name_only)
        {
            // Refresh tree, property grid and open windows
            if (name_only)
                ChangeItemLabel(GetNodeID(item), GetNodeLabel(item));
            else
                RePopulateTreeView(GetNodeID(item)); // currently this is the only way to update tree item ids

            foreach (ContentDocument doc in _documents.Values)
            {
                var docItem = ((InventoryEditor)doc.Control).ItemToEdit;
                doc.Name = docItem.WindowTitle;
                _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(docItem), doc, docItem);
            }
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == "Name")
            {
                InventoryItem itemToChange = ((InventoryEditor)_guiController.ActivePane.Control).ItemToEdit;

                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemToChange.ScriptName, itemToChange))
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIcon.Warning);
                    itemToChange.ScriptName = (string)oldValue;
                }
                else
                {
                    OnItemIDOrNameChanged(itemToChange, true);
                }
            }
        }

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New Inventory Item", null));
            if (ClipboardUtils.IsAvailableOnClipboard(typeof(InventoryItem)))
                menu.Add(new MenuCommand(COMMAND_PASTE_ITEM, "Paste Inventory Item", null));
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(MenuCommand.Separator);
                MenuCommand goToCommand = new MenuCommand(COMMAND_GO_TO_ITEM_NUMBER, "Go to Inventory Item...", Keys.Control | Keys.G);
                goToCommand.Enabled = Factory.AGSEditor.CurrentGame.InventoryFlatList.Count > 0;
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
                int invID = Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.RootInventoryItemFolder.FindInventoryItemByID(invID, true);
                if (_itemRightClicked != null)
                {
                    menu.Add(new MenuCommand(COMMAND_CHANGE_ID, "Change inventory ID", null));
                    menu.Add(new MenuCommand(COMMAND_COPY_ITEM, "Copy item", null));
                    if (ClipboardUtils.IsAvailableOnClipboard(typeof(InventoryItem)))
                        menu.Add(new MenuCommand(COMMAND_PASTE_ITEM, "Paste item", null));
                    menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this item", null));
                    menu.Add(new MenuCommand(COMMAND_FIND_ALL_USAGES, "Find All Usages of " + _itemRightClicked.ScriptName, null));
                }
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

        private string GetNodeID(InventoryItem item)
        {
            return ITEM_COMMAND_PREFIX + item.ID;
        }

        private string GetNodeLabel(InventoryItem item)
        {
            return item.ID.ToString() + ": " + item.ScriptName;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(InventoryItem item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item), GetNodeLabel(item), "InventoryIcon");
            return treeItem;
        }

        private Dictionary<string, object> ConstructPropertyObjectList(InventoryItem item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.PropertyGridTitle, item);
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

        protected override IList<InventoryItem> GetFlatList()
        {
            return _agsEditor.CurrentGame.InventoryFlatList;
        }

        private void ScanAndReportMissingEventHandlers(GenericMessagesArgs args)
        {
            var errors = args.Messages;
            //var events = GameObjectEvents.GetEvents(typeof(Character));
            //var scriptFunctions = GameObjectEvents.GetScriptFunctions(inv);
            foreach (InventoryItem inv in _agsEditor.CurrentGame.InventoryItems)
            {
                _agsEditor.Tasks.ScanAndReportMissingEventHandlers(inv, "Inventory", "InventoryItem",
                    inv.ScriptName, inv.ID, inv.ScriptModule, true, errors);
            }
        }
    }
}
