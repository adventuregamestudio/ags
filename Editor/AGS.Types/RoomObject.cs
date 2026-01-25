using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabEvents), PropertyTabScope.Component)]
    [DefaultProperty("Image")]
	public class RoomObject : IComparable<RoomObject>, IChangeNotification, ICustomTypeDescriptor, IToXml
    {
		public const string PROPERTY_NAME_SCRIPT_NAME = "Name";
        public const string PROPERTY_NAME_DESCRIPTION = "Description";

        private int _id;
        private int _image;
        private int _x;
        private int _y;
        private int _transparency = 0;
        private bool _clickable = true;
        private bool _enabled = true;
        private bool _visible = true;
        private int _baseline = 0;
        private bool _solid = false;
        private Rectangle _blockingRect = Rectangle.Empty;
        private string _name = string.Empty;
        private string _description = string.Empty;
        private bool _useRoomAreaScaling;
        private bool _useRoomAreaLighting;
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Objects);
        // Game Events
        private Interactions _interactions = new Interactions(InteractionSchema.Instance);
        private string _onAnyClick = string.Empty;
        private string _onFrameEvent = string.Empty;
        //
        private Room _room;

        public RoomObject(Room room)
        {
            _room = room;
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        public Room Room
        {
            get { return _room; }
        }

        [AGSNoSerialize]
        [Description("The ID number of the object")]
        [Category("Design")]
        [ReadOnly(true)]
        [BrowsableMultiedit(false)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }
        
        [Description("Sprite used to display the object")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set { _image = Math.Max(value, 0); }
        }

        [Description("The blend mode for the object")]
        [Category("Appearance")]
        [TypeConverter(typeof(EnumTypeConverter))]
        [DefaultValue(BlendMode.Normal)]
        public BlendMode BlendMode
        {
            get;
            set;
        }
        
        [Description("This object's transparency (0-100)")]
        [Category("Appearance")]
        [DefaultValue(0)]
        public int Transparency
        {
            get { return _transparency; }
            set { _transparency = Math.Max(0, Math.Min(100, value)); }
        }

        [Description("If true, this object will be initially visible and updated during game update.")]
        [Category("Design")]
        public bool Enabled
        {
            get { return _enabled; }
            set { _enabled = value; }
        }

        [Description("If true, this object will be initially visible.")]
        [Category("Design")]
        [DefaultValue(true)]
        public bool Visible
        {
            get { return _visible; }
            set { _visible = value; }
        }

        [Description("The Y co-ordinate used to determine whether this object is in front of or behind other things. Set as 0 to use own object's Y position as a baseline.")]
        [Category("Design")]
        [DefaultValue(0)]
        public int Baseline
        {
            get { return _baseline; }
            set { _baseline = value; }
        }

        [Description("Determines whether the object can be clicked on, or whether mouse clicks pass straight through it")]
        [Category("Design")]
        [DefaultValue(true)]
        public bool Clickable
        {
            get { return _clickable; }
            set { _clickable = value; }
        }

        [Description("Allows you to manually specify this object's position in the front-to-back z-order, rather than the default behaviour of using its Y co-ordinate.")]
        [Category("Design")]
        [DefaultValue(false)]
        [RefreshProperties(RefreshProperties.All)]
        public bool BaselineOverridden
        {
            get { return _baseline > 0; }
            set
            {
                if ((value) && (_baseline < 1))
                {
                    _baseline = 1;
                }
                if ((!value) && (_baseline > 0))
                {
                    _baseline = 0;
                }
            }
        }

        [Browsable(false)]
        public int EffectiveBaseline
        {
            get { return BaselineOverridden ? Baseline : StartY; }
        }

        [Description("If true, then this object will be preventing solid characters movement")]
        [Category("Design")]
        [DefaultValue(false)]
        public bool Solid
        {
            get { return _solid; }
            set { _solid = value; }
        }

        [Description("Defines a rectangle in relative coordinates around the object's position, which prevents solid characters from walking through.")]
        [Category("Design")]
        public Rectangle BlockingRectangle
        {
            get { return _blockingRect; }
            set { _blockingRect = value; }
        }

        private void ResetBlockingRectangle() => BlockingRectangle = Rectangle.Empty;
        private bool ShouldSerializeBlockingRectangle() => BlockingRectangle != Rectangle.Empty;

        [Description("X co-ordinate within the room of the left side of the object")]
        [Category("Design")]
        public int StartX
        {
            get { return _x; }
            set { _x = value; }
        }

        [Description("Y co-ordinate within the room of the bottom of the object")]
        [Category("Design")]
        public int StartY
        {
            get { return _y; }
            set { _y = value; }
        }

        [Description("Description of the object")]
        [Category("Appearance")]
        [EditorAttribute(typeof(MultiLineStringUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string Description
        {
            get { return _description; }
            set { _description = value; }
        }

		[DisplayName(PROPERTY_NAME_SCRIPT_NAME)]
        [Description("The script name of the object")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
        public string Name
        {
            get { return _name; }
            set { _name = Utilities.ValidateScriptName(value); }
        }

        [Description("Whether the object should be affected by walkable area scaling")]
        [Category("Appearance")]
        public bool UseRoomAreaScaling
        {
            get { return _useRoomAreaScaling; }
            set { _useRoomAreaScaling = value; }
        }

        [Description("Whether the object should be affected by walkable area lighting")]
        [Category("Appearance")]
        public bool UseRoomAreaLighting
        {
            get { return _useRoomAreaLighting; }
            set { _useRoomAreaLighting = value; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return TypesHelper.MakePropertyGridTitle("Object", _name, _description, _id); }
        }

        [AGSSerializeClass()]
        [Description("Custom properties for this object")]
        [AutoExpand]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set
            {
                _properties = value;
                _properties.AppliesTo = CustomPropertyAppliesTo.Objects;
            }
        }

        #region Game Events

        [AGSNoSerialize()]
        [Browsable(false)]
        [Category("Cursor Events")]
        [ScriptFunction("Object *theObject, CursorMode mode")]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        [DisplayName("Any click on")]
        [Category("Cursor Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("AnyClick", "Object *theObject, CursorMode mode")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnAnyClick
        {
            get { return _onAnyClick; }
            set { _onAnyClick = value; }
        }

        [Description("Script function to run when the animating object displays a frame with custom event")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("OnFrameEvent", "Object *theObject, int view, int loop, int frame, String eventName")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnFrameEvent
        {
            get { return _onFrameEvent; }
            set { _onFrameEvent = value; }
        }

        #endregion // Game Events

        #region IComparable<RoomObject> Members

        public int CompareTo(RoomObject other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion

        #region IChangeNotification Members

        void IChangeNotification.ItemModified()
		{
            (_room as IChangeNotification).ItemModified();
		}

        #endregion


        #region ICustomTypeDescriptor Members

        public AttributeCollection GetAttributes()
		{
			return TypeDescriptor.GetAttributes(this, true);
		}

		public string GetClassName()
		{
			return TypeDescriptor.GetClassName(this, true);
		}

		public string GetComponentName()
		{
			return TypeDescriptor.GetComponentName(this, true);
		}

		public TypeConverter GetConverter()
		{
			return TypeDescriptor.GetConverter(this, true);
		}

		public EventDescriptor GetDefaultEvent()
		{
			return TypeDescriptor.GetDefaultEvent(this, true);
		}

		public PropertyDescriptor GetDefaultProperty()
		{
			return TypeDescriptor.GetDefaultProperty(this, true);
		}

		public object GetEditor(Type editorBaseType)
		{
			return TypeDescriptor.GetEditor(this, editorBaseType, true);
		}

		public EventDescriptorCollection GetEvents(Attribute[] attributes)
		{
			return TypeDescriptor.GetEvents(this, attributes, true);
		}

		public EventDescriptorCollection GetEvents()
		{
			return TypeDescriptor.GetEvents(this, true);
		}

		public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
		{
			// We want to hide the X & Y movement speed properties if they aren't
			// using Uniform Movement, and hide the standard movement speed property
			// if they are.
			PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
			List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
			foreach (PropertyDescriptor property in properties)
			{
				bool wantThisProperty = true;
				if ((!this.BaselineOverridden) && (property.Name == "Baseline"))
				{
					wantThisProperty = false;
				}

				if (wantThisProperty)
				{
					wantProperties.Add(property);
				}
			}
			return new PropertyDescriptorCollection(wantProperties.ToArray());
		}

		public PropertyDescriptorCollection GetProperties()
		{
			PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, true);
			return properties;
		}

		public object GetPropertyOwner(PropertyDescriptor pd)
		{
			return this;
		}

        #endregion

        #region Serialization

        public RoomObject(Room room, XmlNode node) : this(room)
        {
            SerializeUtils.DeserializeFromXML(this, node);
            // Disable schema temporarily, in case we must convert from old format
            _interactions.Schema = null;
            _interactions.FromXml(node);
            ConvertOldInteractionEvents();
            _interactions.Schema = InteractionSchema.Instance;
        }

        private void ConvertOldInteractionEvents()
        {
            if (_interactions.IndexedFunctionNames.Count == 0)
                return; // no old indexed events, no conversion necessary

            OnAnyClick = _interactions.IndexedFunctionNames.TryGetValueOrDefault(4, string.Empty);
            _interactions.RemapLegacyFunctionIndexes(new int[]
            {
                -1 /* Walk */, 0 /* Look */, 1 /* Interact */, 2 /* Talk */, 3 /* UseInv */,
                5 /* PickUp */, -1 /* Pointer */, -1 /* Wait */, 6 /* Mode8 */, 7 /* Mode9 */
            });
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            _interactions.ToXml(writer);
            writer.WriteEndElement();
        }

        #endregion // Serialization
    }
}
