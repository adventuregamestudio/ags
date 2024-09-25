using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [Serializable]
    public class CustomProperties : ICloneable
    {
        private Dictionary<string, CustomProperty> _properties = new Dictionary<string, CustomProperty>();

        public CustomProperties()
        {
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

        public object Clone()
        {
            CustomProperties props = new CustomProperties();
            props._properties =
                _properties.ToDictionary(entry => entry.Key,
                                          entry => (CustomProperty)entry.Value.Clone());
            return props;
        }
    }
}
