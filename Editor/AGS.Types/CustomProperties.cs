using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Xml;

namespace AGS.Types
{
    [TypeConverter(typeof(ExpandableObjectConverter))]
    [EditorAttribute(typeof(CustomPropertiesUIEditor), typeof(System.Drawing.Design.UITypeEditor))]

    public class CustomProperties : ICustomTypeDescriptor
    {
        private Dictionary<string,CustomProperty> _properties = new Dictionary<string,CustomProperty>();
        private CustomPropertyAppliesTo _appliesTo;

        static public CustomPropertySchema Schema;

        [AGSNoSerialize()]
        public CustomPropertyAppliesTo AppliesTo
        {
            set { _appliesTo = value; }
            get { return _appliesTo; }
        }

        public CustomProperties(CustomPropertyAppliesTo appliesTo)
        {
            _appliesTo = appliesTo;
        }

        public Dictionary<string, CustomProperty> PropertyValues
        {
            get { return _properties; }
        }

        public CustomProperty this[string propertyName]
        {
            get { return _properties[propertyName]; }
        }

        public CustomProperties(XmlNode propertiesNode)
        {
            foreach (XmlNode child in propertiesNode.ChildNodes)
            {
                CustomProperty newProp = new CustomProperty(child);
                _properties.Add(newProp.Name, newProp);
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Properties");
            foreach (string key in _properties.Keys)
            {
                _properties[key].ToXml(writer);
            }
            writer.WriteEndElement();
        }

        public override string ToString()
        {
            return "(Properties)";
        }

        #region ICustomTypeDescriptor

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

        public EventDescriptorCollection GetEvents()
        {
            return TypeDescriptor.GetEvents(this, true);
        }

        public EventDescriptorCollection GetEvents(Attribute[] attributes)
        {
            return TypeDescriptor.GetEvents(this, attributes, true);
        }

        public PropertyDescriptorCollection GetProperties()
        {
            return GetProperties(null);
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            var descriptors = new List<PropertyDescriptor>();
            foreach (CustomPropertySchemaItem item in Schema.PropertyDefinitions)
            {
                if (((_appliesTo == CustomPropertyAppliesTo.Characters) && (item.AppliesToCharacters)) ||
                    ((_appliesTo == CustomPropertyAppliesTo.Hotspots) && (item.AppliesToHotspots)) ||
                    ((_appliesTo == CustomPropertyAppliesTo.InventoryItems) && (item.AppliesToInvItems)) ||
                    ((_appliesTo == CustomPropertyAppliesTo.Objects) && (item.AppliesToObjects)) ||
                    ((_appliesTo == CustomPropertyAppliesTo.Rooms) && (item.AppliesToRooms)))
                {
                    descriptors.Add(new CustomPropertyDescriptor(item, this));
                }
            }

            return new PropertyDescriptorCollection(descriptors.ToArray());
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }
        #endregion // ICustomTypeDescriptor
    }
}
