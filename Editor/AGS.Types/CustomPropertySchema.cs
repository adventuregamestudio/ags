using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class CustomPropertySchema
    {
        /*
         * Custom property schema history:
         * 
         * 3.00.00.00 - initial 3.* custom properties schema
         * 3.00.00.08 - Translated property in schema items
        */
        public const int LATEST_XML_VERSION_INDEX = 3060308;

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
            if (node.SelectSingleNode("PropertyDefinitions") == null)
                return;

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
