using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabInteractions), PropertyTabScope.Component)]
    [DefaultProperty("Image")]
	public class RoomObject : IComparable<RoomObject>, IChangeNotification, ICustomTypeDescriptor, IToXml
    {
		public const string PROPERTY_NAME_SCRIPT_NAME = "Name";
        public const string PROPERTY_NAME_DESCRIPTION = "Description";

        private static InteractionSchema _interactionSchema;

        private int _id;
        private int _image;
        private int _x;
        private int _y;
        private bool _clickable = true;
        private bool _enabled = true;
        private bool _visible = true;
        private int _baseline;
        private int _effectiveBaseline;
        private string _name = string.Empty;
        private string _description = string.Empty;
        private bool _useRoomAreaScaling;
        private bool _useRoomAreaLighting;
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Objects);
        private Interactions _interactions = new Interactions(_interactionSchema);
        private Room _room;

        static RoomObject()
        {
            _interactionSchema = new InteractionSchema(string.Empty, true,
                new string[] {"$$01 object",
                "$$02 object", "$$03 object",  "Use inventory on object", 
                "Any click on object", 
                "$$05 object", "$$08 object", "$$09 object"},
                new string[] { "Look", "Interact", "Talk", "UseInv", "AnyClick", "PickUp", "Mode8", "Mode9" },
                "Object *theObject, CursorMode mode");
        }

        public RoomObject(Room room)
        {
            _room = room;
        }

        public RoomObject(Room room, XmlNode node) : this(room)
        {
            SerializeUtils.DeserializeFromXML(this, node);
            Interactions.FromXml(node);
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

        [Description("If true, this object will be initially visible and updated during game update.")]
        [Category("Design")]
        public bool Enabled
        {
            get { return _enabled; }
            set { _enabled = value; }
        }

        [Description("If true, this object will be initially visible.")]
        [Category("Design")]
        public bool Visible
        {
            get { return _visible; }
            set { _visible = value; }
        }

        [Description("The Y co-ordinate used to determine whether this object is in front of or behind other things.")]
        [Category("Design")]
        public int Baseline
        {
            get { return _baseline; }
            set { _baseline = value; }
        }

        [Description("Determines whether the object can be clicked on, or whether mouse clicks pass straight through it")]
        [Category("Design")]
        public bool Clickable
        {
            get { return _clickable; }
            set { _clickable = value; }
        }

		[Description("Allows you to manually specify this object's position in the front-to-back z-order, rather than the default behaviour of using its Y co-ordinate.")]
		[Category("Design")]
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
            get { return _effectiveBaseline; }
            set { _effectiveBaseline = value; }
        }

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
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set
            {
                _properties = value;
                _properties.AppliesTo = CustomPropertyAppliesTo.Objects;
            }
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        public int CompareTo(RoomObject other)
        {
            return this.EffectiveBaseline.CompareTo(other.EffectiveBaseline);
        }

		void IChangeNotification.ItemModified()
		{
            (_room as IChangeNotification).ItemModified();
		}


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

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            _interactions.ToXml(writer);
            writer.WriteEndElement();
        }
    }
}
