using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
    [PropertyTab(typeof(PropertyTabInteractions), PropertyTabScope.Component)]
    public class RoomRegion : ICustomTypeDescriptor
    {
        private static InteractionSchema _interactionSchema;

        private int _id;
        private int _lightLevel = 100;
        private bool _useTint;
        private int _redTint;
        private int _greenTint;
        private int _blueTint;
        private int _tintAmount;
        private int _tintLuminance;
        private Interactions _interactions = new Interactions(_interactionSchema);

        static RoomRegion()
        {
            _interactionSchema = new InteractionSchema(new string[] {
                "While standing on region",
                "Walks onto region", 
                "Walks off region"},
                new string[] { "Standing", "WalksOnto", "WalksOff" });
        }

        [Description("The ID number of the region")]
        [Category("Design")]
        [ReadOnly(true)]
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

        [Description("The saturation of the region tint (0=no tint, 100=fully colourize)")]
        [Category("Lighting")]
        public int TintSaturation
        {
            get { return _tintAmount; }
            set { _tintAmount = value; }
        }

        [Description("The luminance of the region tint (0=pitch black, 100=original lighting)")]
        [Category("Lighting")]
        public int TintLuminance
        {
            get { return _tintLuminance; }
            set { _tintLuminance = value; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return "Region ID " + _id; }
        }

        [AGSNoSerialize()]
        [Browsable(false)]
        public Interactions Interactions
        {
            get { return _interactions; }
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

    }
}
