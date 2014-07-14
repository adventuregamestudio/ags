using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class InventoryItemFolders : FolderListHybrid<InventoryItem, InventoryItemFolder>
    {
        public InventoryItemFolders() : base(new InventoryItemFolder()) { }

        public InventoryItemFolders(string name) : base(new InventoryItemFolder(name)) { }

        public InventoryItemFolders(XmlNode node, XmlNode parentNodeForBackwardsCompatability) :
            base(new InventoryItemFolder(node, parentNodeForBackwardsCompatability)) { }
    }

    public class InventoryItemFolder : BaseFolderCollection<InventoryItem, InventoryItemFolder>
    {
        public const string MAIN_INVENTORY_ITEM_FOLDER_NAME = "Main";

        public InventoryItemFolder(string name) : base(name) { }

        public InventoryItemFolder() : this("Default") { }

        public InventoryItemFolder(XmlNode node, XmlNode parentNodeForBackwardsCompatability) :
            base(node, parentNodeForBackwardsCompatability) { }

        private InventoryItemFolder(XmlNode node) : base(node) { }

        public override InventoryItemFolder CreateChildFolder(string name)
        {
            return new InventoryItemFolder(name);
        }

        public InventoryItem FindInventoryItemByID(int inventoryItemID, bool recursive)
        {
            return FindItem(IsItem, inventoryItemID, recursive);
        }

        protected override void FromXmlBackwardsCompatability(System.Xml.XmlNode parentNodeForBackwardsCompatability)
        {
            Init(MAIN_INVENTORY_ITEM_FOLDER_NAME);
            foreach (XmlNode inventoryItemNode in SerializeUtils.GetChildNodes(parentNodeForBackwardsCompatability, "InventoryItems"))
            {
                _items.Add(new InventoryItem(inventoryItemNode));
            }
        }

        protected override InventoryItemFolder CreateFolder(XmlNode node)
        {
            return new InventoryItemFolder(node);
        }

        protected override InventoryItem CreateItem(XmlNode node)
        {
            return new InventoryItem(node);
        }

        private bool IsItem(InventoryItem inventoryItem, int inventoryItemID)
        {
            return inventoryItem.ID == inventoryItemID;
        }
    }
}
