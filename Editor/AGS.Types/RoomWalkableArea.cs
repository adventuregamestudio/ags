using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;

namespace AGS.Types
{
    public class RoomWalkableArea : ICustomTypeDescriptor
    {
        private int _id;
        private int _areaSpecificView;
        private int _scalingLevel = 100;
        private bool _useContinuousScaling;
        private int _scalingLevelMin = 100;
        private int _scalingLevelMax = 100;

        [Description("The ID number of the walkable area")]
        [Category("Design")]
        [ReadOnly(true)]
        public int ID
        {
            get { return _id; }
            set { _id = value; }
        }

        [Description("The player character's walking view will switch to this when he is standing on the area")]
        [Category("Design")]
        [DefaultValue(0)]
        [EditorAttribute(typeof(ViewUIEditor), typeof(System.Drawing.Design.UITypeEditor))]
        public int AreaSpecificView
        {
            get { return _areaSpecificView; }
            set { _areaSpecificView = value; }
        }

        [Description("Enables this area to have a graduated scaling level from the top to bottom, rather than a fixed scaling.")]
        [Category("Scaling")]
        [DefaultValue(false)]
        [RefreshProperties(RefreshProperties.All)]
        public bool UseContinuousScaling
        {
            get { return _useContinuousScaling; }
            set { _useContinuousScaling = value; }
        }

        [Description("Scaling level for this area (100 is unscaled)")]
        [Category("Scaling")]
        [DefaultValue(100)]
        public int ScalingLevel
        {
            get { return _scalingLevel; }
            set { _scalingLevel = value; }
        }

        [Description("Scaling level at the top of this area")]
        [Category("Scaling")]
        public int MinScalingLevel
        {
            get { return _scalingLevelMin; }
            set { _scalingLevelMin = value; }
        }

        [Description("Scaling level at the bottom of this area")]
        [Category("Scaling")]
        public int MaxScalingLevel
        {
            get { return _scalingLevelMax; }
            set { _scalingLevelMax = value; }
        }

        [Browsable(false)]
        public string PropertyGridTitle
        {
            get { return "Walkable area ID " + _id; }
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
            // We want to hide the min & max scaling properties if they aren't
            // using continuous scaling, and hide the standard scaling property
            // if they are.
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                bool wantThisProperty = true;
                if ((!_useContinuousScaling) &&
                    ((property.Name == "MinScalingLevel") || (property.Name == "MaxScalingLevel")))
                {
                    wantThisProperty = false;
                }
                else if ((_useContinuousScaling) && (property.Name == "ScalingLevel"))
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
