using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class DialogFolders : FolderListHybrid<DialogRef, DialogFolder>
    {
        public DialogFolders() : base(new DialogFolder()) { }

        public DialogFolders(string name) : base(new DialogFolder(name)) { }

        public DialogFolders(XmlNode node, XmlNode parentNodeForBackwardsCompatability, System.Version xmlVersion) :
            base(new DialogFolder(node, parentNodeForBackwardsCompatability, xmlVersion)) { }
    }

    public class DialogFolder : BaseFolderCollection<DialogRef, DialogFolder>
    {
        public const string MAIN_DIALOG_FOLDER_NAME = "Main";        

        public DialogFolder(string name) : base(name) { }

        public DialogFolder() : this("Default") { }

        public DialogFolder(XmlNode node, XmlNode parentNodeForBackwardsCompatability, System.Version xmlVersion) : 
            base(node, parentNodeForBackwardsCompatability, xmlVersion) { }

        private DialogFolder(XmlNode node) : base(node) { }

        protected override string OverrideXmlItemListNodeName()
        {
            return "Dialogs";
        }

        public override DialogFolder CreateChildFolder(string name)
        {
            return new DialogFolder(name);
        }

        public DialogRef FindDialogByID(int dialogID, bool recursive)
        {
            return FindItem(IsItem, dialogID, recursive);
        }

        protected override void FromXmlBackwardsCompatability(System.Xml.XmlNode parentNodeForBackwardsCompatability, System.Version xmlVersion)
        {
            Init(MAIN_DIALOG_FOLDER_NAME);
            foreach (XmlNode dialogNode in SerializeUtils.GetChildNodesOrEmpty(parentNodeForBackwardsCompatability, "Dialogs"))
            {
                _items.Add(new DialogRef(dialogNode, xmlVersion));
            }
        }

        protected override DialogFolder CreateFolder(XmlNode node)
        {
            return new DialogFolder(node);
        }

        protected override DialogRef CreateItem(XmlNode node, System.Version xmlVersion)
        {
            return new DialogRef(node, xmlVersion);
        }

        private bool IsItem(DialogRef dialog, int dialogID)
        {
            return dialog.ID == dialogID;
        }
    }
}
