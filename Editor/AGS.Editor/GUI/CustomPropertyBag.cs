using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using AGS.Types;

namespace AGS.Editor
{
    internal class CustomPropertyBag : ICustomTypeDescriptor
    {
        private CustomProperties _properties;
        private CustomPropertySchema _schema;
        private CustomPropertyAppliesTo _showPropertiesThatApplyTo;

        public CustomPropertyBag(CustomPropertySchema schema, CustomProperties properties, CustomPropertyAppliesTo showPropertiesThatApplyTo)
        {
            _properties = properties;
            _schema = schema;
            _showPropertiesThatApplyTo = showPropertiesThatApplyTo;
        }

        public AttributeCollection GetAttributes()
        {
            return AttributeCollection.Empty;
        }

        public string GetClassName()
        {
            return null;
        }

        public string GetComponentName()
        {
            return null;
        }

        public TypeConverter GetConverter()
        {
            return null;
        }

        public EventDescriptor GetDefaultEvent()
        {
            return null;
        }

        public PropertyDescriptor GetDefaultProperty()
        {
            return null;
        }

        public object GetEditor(Type editorBaseType)
        {
            return null;
        }

        public EventDescriptorCollection GetEvents(Attribute[] attributes)
        {
            return EventDescriptorCollection.Empty;
        }

        public EventDescriptorCollection GetEvents()
        {
            return EventDescriptorCollection.Empty;
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            List<PropertyDescriptor> descriptors = new List<PropertyDescriptor>();
            foreach (CustomPropertySchemaItem item in _schema.PropertyDefinitions)
            {
                if (((_showPropertiesThatApplyTo == CustomPropertyAppliesTo.Characters) && (item.AppliesToCharacters)) ||
                    ((_showPropertiesThatApplyTo == CustomPropertyAppliesTo.Hotspots) && (item.AppliesToHotspots)) ||
                    ((_showPropertiesThatApplyTo == CustomPropertyAppliesTo.InventoryItems) && (item.AppliesToInvItems)) ||
                    ((_showPropertiesThatApplyTo == CustomPropertyAppliesTo.Objects) && (item.AppliesToObjects)) ||
                    ((_showPropertiesThatApplyTo == CustomPropertyAppliesTo.Rooms) && (item.AppliesToRooms)))
                {
                    PropertyDescriptor descriptor = new CustomPropertyDescriptor(item, _properties);
                    descriptors.Add(descriptor);
                }
            }
            return new PropertyDescriptorCollection(descriptors.ToArray());
        }

        public PropertyDescriptorCollection GetProperties()
        {
            throw new Exception("The method or operation is not implemented.");
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }
    }
}
