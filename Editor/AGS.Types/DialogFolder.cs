using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class DialogFolders : FolderListHybrid<Dialog, DialogFolder>
    {
        public DialogFolders() : base(new DialogFolder()) { }

        public DialogFolders(string name) : base(new DialogFolder(name)) { }

        public DialogFolders(XmlNode node, XmlNode parentNodeForBackwardsCompatability) :
            base(new DialogFolder(node, parentNodeForBackwardsCompatability)) { }
    }

    public class DialogFolder : BaseFolderCollection<Dialog, DialogFolder>
    {
        public const string MAIN_DIALOG_FOLDER_NAME = "Main";        

        public DialogFolder(string name) : base(name) { }

        public DialogFolder() : this("Default") { }

        public DialogFolder(XmlNode node, XmlNode parentNodeForBackwardsCompatability) : 
            base(node, parentNodeForBackwardsCompatability) { }

        private DialogFolder(XmlNode node) : base(node) { }

        public override DialogFolder CreateChildFolder(string name)
        {
            return new DialogFolder(name);
        }

        public Dialog FindDialogByID(int dialogID, bool recursive)
        {
            return FindItem(IsItem, dialogID, recursive);
        }

        protected override void FromXmlBackwardsCompatability(System.Xml.XmlNode parentNodeForBackwardsCompatability)
        {
            Init(MAIN_DIALOG_FOLDER_NAME);
            foreach (XmlNode dialogNode in SerializeUtils.GetChildNodes(parentNodeForBackwardsCompatability, "Dialogs"))
            {
                _items.Add(new Dialog(dialogNode));
            }
        }

        protected override DialogFolder CreateFolder(XmlNode node)
        {
            return new DialogFolder(node);
        }

        protected override Dialog CreateItem(XmlNode node)
        {
            return new Dialog(node);
        }

        private bool IsItem(Dialog dialog, int dialogID)
        {
            return dialog.ID == dialogID;
        }
    }
}
