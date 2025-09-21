using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabEvents), PropertyTabScope.Component)]
    [DefaultProperty("LightLevel")]
    public class RoomRegion : IChangeNotification, ICustomTypeDescriptor, IToXml
    {
        private int _id;
        private int _lightLevel = 100;
        private bool _useTint = false;
        private int _redTint = 0;
        private int _greenTint = 0;
        private int _blueTint = 0;
        private int _tintAmount = 50;
        private int _tintLuminance = 100;
        private CustomProperties _properties = new CustomProperties(CustomPropertyAppliesTo.Regions);
        // Game Events
        private Interactions _interactions = new Interactions(InteractionSchema.Instance);
        private string _onStanding;
        private string _onWalksOnto;
        private string _onWalksOff;
        //
        private Room _room;

        public RoomRegion(Room room)
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
        [Description("The ID number of the region")]
        [Category("Design")]
        [ReadOnly(true)]
        [BrowsableMultiedit(false)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("Use a coloured tint for characters and objects on this region, rather than adjusting their brightness")]
        [Category("Lighting")]
        [DefaultValue(false)]
        [RefreshProperties(RefreshProperties.All)]
        public bool UseColourTint
        {
            get { return _useTint; }
            set { _useTint = value; }
        }

        [Description("Light level for this region (100 is normal)")]
        [Category("Lighting")]
        [DefaultValue(100)]
        public int LightLevel
        {
            get { return _lightLevel; }
            set { _lightLevel = value; }
        }

        [Description("The Red component of the region tint")]
        [Category("Lighting")]
        public int RedTint
        {
            get { return _redTint; }
            set { _redTint = value; }
        }

        [Description("The Green component of the region tint")]
        [Category("Lighting")]
        public int GreenTint
        {
            get { return _greenTint; }
            set { _greenTint = value; }
        }

        [Description("The Blue component of the region tint")]
        [Category("Lighting")]
        public int BlueTint
        {
            get { return _blueTint; }
            set { _blueTint = value; }
        }

        [Description("The saturation of the region tint (1=minimal tint, 100=fully colourize)")]
        [Category("Lighting")]
        [DefaultValue(50)]
        public int TintSaturation
        {
            get { return _tintAmount; }
            set
            {
                if (value == 0)
                    throw new ArgumentOutOfRangeException("Saturation cannot be 0, disable colour tint instead");
                _tintAmount = value;
            }
        }

        [Description("The luminance of the region tint (0=pitch black, 100=original lighting)")]
        [Category("Lighting")]
        [DefaultValue(100)]
        public int TintLuminance
        {
            get { return _tintLuminance; }
            set { _tintLuminance = value; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return TypesHelper.MakePropertyGridTitle("Region", _id); }
        }

        #region Game Events

        [AGSNoSerialize()]
        [Browsable(false)]
        [Obsolete]
        public Interactions Interactions
        {
            get { return _interactions; }
        }

        [DisplayName("While standing on region")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("Standing", "Region *theRegion")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnStanding
        {
            get { return _onStanding; }
            set { _onStanding = value; }
        }

        [DisplayName("Walks onto region")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("WalksOnto", "Region *theRegion")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnWalksOnto
        {
            get { return _onWalksOnto; }
            set { _onWalksOnto = value; }
        }

        [DisplayName("Walks off region")]
        [Category("Events")]
        [Browsable(false)]
        [AGSEventsTabProperty(), AGSEventProperty()]
        [ScriptFunction("WalksOff", "Region *theRegion")]
        [EditorAttribute(typeof(ScriptFunctionUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public string OnWalksOff
        {
            get { return _onWalksOff; }
            set { _onWalksOff = value; }
        }

        #endregion // Game Events

        [AGSSerializeClass()]
        [Description("Custom properties for this region")]
        [Category("Properties")]
        [EditorAttribute(typeof(CustomPropertiesUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public CustomProperties Properties
        {
            get { return _properties; }
            protected set
            {
                _properties = value;
                _properties.AppliesTo = CustomPropertyAppliesTo.Regions;
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
            // We want to hide the tint-related properties if they aren't
            // using tinting, and hide the lighting property if they are.
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                bool wantThisProperty = true;
                if ((!_useTint) &&
                    ((property.Name == "RedTint") || 
                    (property.Name == "GreenTint") ||
                    (property.Name == "BlueTint") ||
                    (property.Name == "TintSaturation") ||
                    (property.Name == "TintLuminance")))
                {
                    wantThisProperty = false;
                }
                else if ((_useTint) && (property.Name == "LightLevel"))
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

        void IChangeNotification.ItemModified()
        {
            (_room as IChangeNotification).ItemModified();
        }

        #region Serialization

        public RoomRegion(Room room, XmlNode node) : this(room)
        {
            SerializeUtils.DeserializeFromXML(this, node);

            LoadAndConvertInteractionEvents(node);
        }

        private void LoadAndConvertInteractionEvents(XmlNode node)
        {
            if (node.SelectSingleNode("Interactions") == null)
                return;

            // Load old-style events into the temporary interactions object,
            // as the one that we are keeping is for compatibility only.
            Interactions interactions = new Interactions(null);
            interactions.FromXml(node);
            if (interactions.IndexedFunctionNames.Count == 0)
                return; // no old indexed events, bail out
            // Convert interaction events to our new event properties
            OnStanding = interactions.IndexedFunctionNames.TryGetValueOrDefault(0, string.Empty);
            OnWalksOnto = interactions.IndexedFunctionNames.TryGetValueOrDefault(1, string.Empty);
            OnWalksOff = interactions.IndexedFunctionNames.TryGetValueOrDefault(2, string.Empty);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer, false);
            writer.WriteEndElement();
        }

        #endregion // Serialization
    }
}
