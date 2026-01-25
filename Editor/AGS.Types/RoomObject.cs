using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabInteractions), PropertyTabScope.Component)]
    [DefaultProperty("Image")]
	public class RoomObject : IComparable<RoomObject>, IChangeNotification, ICustomTypeDescriptor
    {
		public const string PROPERTY_NAME_SCRIPT_NAME = "Name";
        public const string PROPERTY_NAME_DESCRIPTION = "Description";

        private static InteractionSchema _interactionSchema;

        private int _id;
        private int _image;
        private int _x;
        private int _y;
        private int _transparency = 0;
        private bool _clickable = true;
        private bool _visible = true;
        private int _baseline = 0;
        private bool _solid = false;
        private Rectangle _blockingRect = Rectangle.Empty;
        private string _name = string.Empty;
        private string _description = string.Empty;
        private bool _useRoomAreaScaling;
        private bool _useRoomAreaLighting;
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Objects);
        private Interactions _interactions = new Interactions(_interactionSchema);
		private IChangeNotification _notifyOfModification;

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

		public RoomObject(IChangeNotification changeNotifier)
		{
			_notifyOfModification = changeNotifier;
		}

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

        [Description("This object's transparency (0-100)")]
        [Category("Appearance")]
        [DefaultValue(0)]
        public int Transparency
        {
            get { return _transparency; }
            set { _transparency = Math.Max(0, Math.Min(100, value)); }
        }

        [Description("Is the object initially visible?")]
        [Category("Design")]
        [DefaultValue(true)]
        public bool Visible
        {
            get { return _visible; }
            set { _visible = value; }
        }

        [Description("Determines whether the object can be clicked on, or whether mouse clicks pass straight through it")]
        [Category("Design")]
        [DefaultValue(true)]
        public bool Clickable
        {
            get { return _clickable; }
            set { _clickable = value; }
        }

        [Description("The Y co-ordinate used to determine whether this object is in front of or behind other things. Set as 0 to use own object's Y position as a baseline.")]
        [Category("Design")]
        [DefaultValue(0)]
        [RefreshProperties(RefreshProperties.All)]
        public int Baseline
        {
            get { return _baseline; }
            set { _baseline = value; }
        }

        [AGSNoSerialize]
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

        [AGSNoSerialize()]
        [Browsable(false)]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        #region IComparable<RoomObject> Members

        public int CompareTo(RoomObject other)
        {
            return ID.CompareTo(other.ID);
        }

        #endregion

        #region IChangeNotification Members

        void IChangeNotification.ItemModified()
		{
			_notifyOfModification.ItemModified();
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
	}
}
