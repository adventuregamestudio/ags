using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class GUIFolder : BaseFolderCollection<GUI, GUIFolder>
    {
        public const string MAIN_GUI_FOLDER_NAME = "Main";

        public GUIFolder(string name) : base(name) { }

        public GUIFolder() : this("Default") { }

        public GUIFolder(XmlNode node, XmlNode parentNodeForBackwardsCompatability) : 
            base(node, parentNodeForBackwardsCompatability) { }

        private GUIFolder(XmlNode node) : base(node) { }

        public override GUIFolder CreateChildFolder(string name)
        {
            return new GUIFolder(name);
        }

        public GUI FindGUIByID(int GUIID, bool recursive)
        {
            return FindItem(IsItem, GUIID, recursive);
        }

        protected override void FromXmlBackwardsCompatability(System.Xml.XmlNode parentNodeForBackwardsCompatability)
        {
            Init(MAIN_GUI_FOLDER_NAME);
            foreach (XmlNode guiNode in SerializeUtils.GetChildNodes(parentNodeForBackwardsCompatability, "GUIs"))
            {
                if (guiNode.FirstChild.Name == NormalGUI.XML_ELEMENT_NAME)
                {
                    _items.Add(new NormalGUI(guiNode));
                }
                else
                {
                    _items.Add(new TextWindowGUI(guiNode));
                }
            }
        }

        protected override GUIFolder CreateFolder(XmlNode node)
        {
            return new GUIFolder(node);
        }

        protected override GUI CreateItem(XmlNode node)
        {            
            if (node.FirstChild.Name == NormalGUI.XML_ELEMENT_NAME)
                return new NormalGUI(node);
            else return new TextWindowGUI(node);                    
        }
         
        private bool IsItem(GUI GUI, int GUIID)
        {
            return GUI.ID == GUIID;
        }

    }
}
