using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabEvents), PropertyTabScope.Component)]
    [DefaultProperty("Image")]
    public class InventoryItem : IToXml, IComparable<InventoryItem>
    {
        private string _name;
        private string _description;
        private int _image;
        private int _cursorImage;
        private bool _startWithItem;
        private int _id;
        private int _hotspotX, _hotspotY;
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.InventoryItems);
        // Game Events
        private string _scriptModule = Script.GLOBAL_SCRIPT_FILE_NAME;
        private Interactions _interactions = new Interactions(InteractionSchema.Instance);
        private string _onAnyClick;
        //
        private bool _currentlyDeserializing = false;

        public InventoryItem()
        {
            _startWithItem = false;
            _name = string.Empty;
            _description = string.Empty;
            _image = 0;
            _cursorImage = 0;
            _hotspotX = 0;
            _hotspotY = 0;
        }

        [Description("The ID number of the item")]
        [Category("Design")]
        [ReadOnly(true)]
        [BrowsableMultiedit(false)]
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
                    _cursorImage = Math.Max(0, value);
                }
            }
        }

        [Description("Sprite used to display the inventory item")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set { _image = Math.Max(0, value); }
        }

        [Description("Description of the item")]
        [Category("Appearance")]
        [EditorAttribute(typeof(MultiLineStringUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string Description
        {
            get { return _description; }
            set { _description = value; }
        }

        [Description("The script name of the item")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
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

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return TypesHelper.MakePropertyGridTitle("Inventory item", _name, _description, _id); }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this item")]
        [Category("Properties")]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set 
            {
                _properties = value;
                _properties.AppliesTo = CustomPropertyAppliesTo.InventoryItems;
            }
        }

        #region Game Events

        [Description("Script module which contains this inventory item's event functions")]
        [Category("(Basic)")]
        [Browsable(false)]
        [AGSEventsTabProperty()]
        [TypeConverter(typeof(ScriptListTypeConverter))]
        public string ScriptModule
        {
            get { return _scriptModule; }
            set { _scriptModule = value; }
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        [Category("Cursor Events")]
        [ScriptFunction("InventoryItem *theItem, CursorMode mode")]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        [DisplayName("Any click on")]
        [Category("Cursor Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("AnyClick", "InventoryItem *theItem, CursorMode mode")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnAnyClick
        {
            get { return _onAnyClick; }
            set { _onAnyClick = value; }
        }

        #endregion // Game Events

        #region Serialization

        public InventoryItem(XmlNode node)
        {
            _currentlyDeserializing = true;

            _cursorImage = -1;

            SerializeUtils.DeserializeFromXML(this, node);

            if (_cursorImage == -1)
            {
                _cursorImage = _image;
            }

            // Disable schema temporarily, in case we must convert from old format
            _interactions.Schema = null;
            _interactions.FromXml(node);
            ConvertOldInteractionEvents();
            _interactions.Schema = InteractionSchema.Instance;

            _currentlyDeserializing = false;
        }

        private void ConvertOldInteractionEvents()
        {
            if (_interactions.IndexedFunctionNames.Count == 0)
                return; // no old indexed events, no conversion necessary

            // Inventory Items had "Other Click" event that fired for virtually any cursor
            // besides Look, Interact and UseInv.
            _interactions.RemapLegacyFunctionIndexes(new int[]
            {
                4 /* Walk */, 0 /* Look */, 1 /* Interact */, 2 /* Talk */, 3 /* UseInv */,
                4 /* PickUp */, 4 /* Pointer */, 4 /* Wait */, 4 /* Mode8 */, 4 /* Mode9 */
            });
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            _interactions.ToXml(writer);
            writer.WriteEndElement();
        }

        #endregion // Serialization

        #region IComparable<InventoryItem> Members

        public int CompareTo(InventoryItem other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion
    }
}
