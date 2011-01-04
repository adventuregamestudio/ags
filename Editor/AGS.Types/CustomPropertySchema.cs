using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class CustomPropertySchema
    {
        private List<CustomPropertySchemaItem> _propertyDefinitions = new List<CustomPropertySchemaItem>();

        public CustomPropertySchema()
        {
        }

        public List<CustomPropertySchemaItem> PropertyDefinitions
        {
            get { return _propertyDefinitions; }
        }

        public void FromXml(XmlNode node)
        {
            _propertyDefinitions.Clear();
            foreach (XmlNode child in SerializeUtils.GetChildNodes(node, "PropertyDefinitions"))
            {
                _propertyDefinitions.Add(new CustomPropertySchemaItem(child));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("PropertyDefinitions");
            foreach (CustomPropertySchemaItem item in _propertyDefinitions)
            {
                item.ToXml(writer);
            }
            writer.WriteEndElement();
        }
    }
}
