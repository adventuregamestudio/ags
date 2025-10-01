using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("Image")]
    public class MouseCursor : ICustomTypeDescriptor
    {
        private string _name = string.Empty;
        private bool _standardMode = false;
        private bool _createEvent = false;
        private string _eventLabel = string.Empty;
        private string _eventFunctionName = string.Empty;
        private int _id = 0;
        private int _image = 0;
        private int _hotspotX = 0, _hotspotY = 0;
        private int _view = 0;
        private bool _animate = false;
        private bool _animateOnlyOnHotspot = false;
        private bool _animateOnlyWhenMoving = false;
        private int _animateDelay = 5;

        public MouseCursor()
        {
        }

        [Description("The ID number of the cursor")]
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

        [Description("This cursor mode should fire interactions via ProcessClick")]
        [Category("Design")]
        public bool StandardMode
        {
            get { return _standardMode; }
            set { _standardMode = value; }
        }

        [Description("This cursor mode creates interaction event for characters and other objects.")]
        [Category("Design")]
        [RefreshProperties(RefreshProperties.All)]
        public bool CreateEvent
        {
            get { return _createEvent; }
            set { _createEvent = value; }
        }

        [Description("The label used for this event in the Property Grid. If left empty then cursor's own Name will be used instead")]
        [Category("Design")]
        public string EventLabel
        {
            get { return _eventLabel; }
            set { _eventLabel = value; }
        }

        [Description("The name which will be used when generating a script function for this event. If left empty then cursor's own Name will be used instead")]
        [Category("Design")]
        public string EventFunctionName
        {
            get { return _eventFunctionName; }
            set { _eventFunctionName = value; }
        }

        [Description("Sprite used to display the cursor")]
        [Category("Appearance")]
        [EditorAttribute(typeof(SpriteSelectUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int Image
        {
            get { return _image; }
            set
            {
                if (value != _image)
                {
                    if (_image != 0)
                    {
                        _hotspotX = 0;
                        _hotspotY = 0;
                    }
                    _image = Math.Max(0, value);
                }
            }
        }

        [Description("The view used to animate the cursor")]
        [Category("Appearance")]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int View
        {
            get { return _view; }
            set { _view = Math.Max(0, value); }
        }

        [Description("Whether the cursor will animate using the specified view")]
        [Category("Appearance")]
        public bool Animate
        {
            get { return _animate; }
            set { _animate = value; }
        }

        [Description("The cursor will only animate when over a hotspot or object (Animate must be set to true)")]
        [Category("Appearance")]
        public bool AnimateOnlyOnHotspots
        {
            get { return _animateOnlyOnHotspot; }
            set { _animateOnlyOnHotspot = value; }
        }

        [Description("The cursor will only animate when it is moving")]
        [Category("Appearance")]
        public bool AnimateOnlyWhenMoving
        {
            get { return _animateOnlyWhenMoving; }
            set { _animateOnlyWhenMoving = value; }
        }

        [Description("Delay between changing frames whilst animating")]
        [Category("Appearance")]
        public int AnimationDelay
        {
            get { return _animateDelay; }
            set { _animateDelay = value; }
        }

        [Description("The name of the cursor")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
        public string Name
        {
            get { return _name; }
            set
            {
                _name = value;
                if (_name.Length > 9) _name = _name.Substring(0, 9);
            }
        }

        [Browsable(false)]
        public string WindowTitle
        {
            get { return "Cursor: " + this.Name; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return TypesHelper.MakePropertyGridTitle("Character", _name, _id); }
        }

        [AGSNoSerialize]
        [Description("The script ID of the cursor")]
        [Category("Design")]
        [BrowsableMultiedit(false)]
        public string ScriptID
        {
			get
			{
				string cursorName = string.Empty;
				for (int i = 0; i < _name.Length; i++)
				{
					if (_name[i].IsScriptWordChar())
					{
						cursorName += _name[i];
					}
				}
				if (cursorName.Length > 0)
				{
					cursorName = "eMode" + cursorName;
				}
				return cursorName;
			}
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
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                bool wantThisProperty = true;
                if ((property.Name == "EventLabel") || (property.Name == "EventFunctionName"))
                {
                    wantThisProperty = _createEvent;
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

        public MouseCursor(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

    }
}
