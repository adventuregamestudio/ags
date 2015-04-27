using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [Serializable]
    [DefaultProperty("CharacterID")]
    public class GUIInventory : GUIControl
    {
        public const string CONTROL_DISPLAY_NAME = "InventoryWindow";
        public const string SCRIPT_CLASS_TYPE = "InvWindow";

        public GUIInventory(int x, int y, int width, int height)
            : base(x, y, width, height)
        {
            _characterID = -1;
            _itemHeight = 20;
            _itemWidth = 30;
        }

        public GUIInventory(XmlNode node) : base(node)
        {
        }

        public GUIInventory() { }

        private int _characterID;
        private int _itemHeight;
        private int _itemWidth;

        [Description("The width in pixels of each inventory item")]
        [Category("Appearance")]
        public int ItemWidth
        {
            get { return _itemWidth; }
            set { _itemWidth = value; }
        }

        [Description("The height in pixels of each inventory item")]
        [Category("Appearance")]
        public int ItemHeight
        {
            get { return _itemHeight; }
            set { _itemHeight = value; }
        }

        [Description("Which character's inventory to display (-1 for player character)")]
        [Category("Setup")]
        public int CharacterID
        {
            get { return _characterID; }
            set { _characterID = value; }
        }

        public override string ControlType
        {
            get
            {
                return CONTROL_DISPLAY_NAME;
            }
        }

        public override string ScriptClassType
        {
            get
            {
                return SCRIPT_CLASS_TYPE;
            }
        }
    }
}