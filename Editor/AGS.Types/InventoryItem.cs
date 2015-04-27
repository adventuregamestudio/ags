using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabInteractions), PropertyTabScope.Component)]
    [DefaultProperty("Image")]
    public class InventoryItem : IToXml, IComparable<InventoryItem>
    {
        private static InteractionSchema _interactionSchema;

        private string _name;
        private string _description;
        private int _image;
        private int _cursorImage;
        private bool _startWithItem;
        private int _id;
        private int _hotspotX, _hotspotY;
        private CustomProperties _properties;
        private Interactions _interactions = new Interactions(_interactionSchema);
        private bool _currentlyDeserializing = false;

        static InventoryItem()
        {
            _interactionSchema = new InteractionSchema(new string[] { "$$01 inventory item", 
                "$$02 inventory item", "$$03 inventory item", "Use inventory on this item", 
                "Other click on inventory item" },
                new string[] { "Look", "Interact", "Talk", "UseInv", "OtherClick" });
        }

        public InventoryItem()
        {
            _startWithItem = false;
            _name = string.Empty;
            _description = string.Empty;
            _image = 0;
            _cursorImage = 0;
            _hotspotX = 0;
            _hotspotY = 0;
            _properties = new CustomProperties();
        }

        [Description("The ID number of the item")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The X location of the cursor hotspot")]
        [Category("Design")]
        public int HotspotX
        {
            get { return _hotspotX; }
            set { _hotspotX = value; }
        }

        [Description("The Y location of the cursor hotspot")]
        [Category("Design")]
        public int HotspotY
        {
            get { return _hotspotY; }
            set { _hotspotY = value; }
        }

        [Description("If true, the player character starts with this in their inventory")]
        [Category("Design")]
        public bool PlayerStartsWithItem
        {
            get { return _startWithItem; }
            set { _startWithItem = value; }
        }

        [Description("Sprite used as the mouse cursor when this item is the active cursor")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int CursorImage
        {
            get { return _cursorImage; }
            set
            {
                if (value != _cursorImage)
                {
                    if ((_cursorImage != 0) && (!_currentlyDeserializing))
                    {
                        _hotspotX = 0;
                        _hotspotY = 0;
                    }
                    _cursorImage = value;
                }
            }
        }

        [Description("Sprite used to display the inventory item")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set { _image = value; }
        }

        [Description("Description of the item")]
        [Category("Appearance")]
        public string Description
        {
            get { return _description; }
            set { _description = value; }
        }

        [Description("The script name of the item")]
        [Category("Design")]
        public string Name
        {
            get { return _name; }
            set { _name = Utilities.ValidateScriptName(value); }
        }

        [Browsable(false)]
        public string WindowTitle
        {
            get { return "Inventory: " + this.Name; }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this item")]
        [Category("Properties")]
        [EditorAttribute(typeof(CustomPropertiesUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set { _properties = value; }
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        public InventoryItem(XmlNode node)
        {
            _currentlyDeserializing = true;

            _cursorImage = -1;

            SerializeUtils.DeserializeFromXML(this, node);

            if (_cursorImage == -1)
            {
                _cursorImage = _image;
            }

            _interactions.FromXml(node);

            _currentlyDeserializing = false;
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            _interactions.ToXml(writer);
            writer.WriteEndElement();
        }

        #region IComparable<InventoryItem> Members

        public int CompareTo(InventoryItem other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion
    }
}
