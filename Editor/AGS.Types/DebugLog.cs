using AGS.Types.Enums;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class DebugLog : ICustomTypeDescriptor
    {
        private LogGroups _logOutput = new LogGroups();
        private LogGroups _logFilter = new LogGroups();

        [DisplayName("Log Output")]
        [TypeConverter(typeof(ExpandableObjectConverter))]
        public LogGroups LogOutput
        {
            get { return _logOutput; }
            set { _logOutput = value; }
        }

        [DisplayName("Log Filter")]
        [TypeConverter(typeof(ExpandableObjectConverter))]
        public LogGroups LogFilter
        {
            get { return _logFilter; }
            set { _logFilter = value; }
        }

        public void SetDefaults()
        {
            _logOutput.SetDefaults();
            _logFilter.SetDefaults();
        }

        public void FromXml(XmlNode node)
        {
            if (node.SelectSingleNode("LogOutput") != null)
            {
                XmlNode logOutputNode = node.SelectSingleNode("LogOutput");
                _logOutput.FromXml(logOutputNode);
            }

            if (node.SelectSingleNode("LogFilter") != null)
            {
                XmlNode logFilterNode = node.SelectSingleNode("LogFilter");
                _logFilter.FromXml(logFilterNode);
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("LogOutput");
            _logOutput.ToXml(writer);
            writer.WriteEndElement();
            writer.WriteStartElement("LogFilter");
            _logFilter.ToXml(writer);
            writer.WriteEndElement();
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
            return TypeDescriptor.GetProperties(this, true);
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            PropertyDescriptorCollection properties = TypeDescriptor.GetProperties(this, attributes, true);
            List<PropertyDescriptor> wantedProperties = new List<PropertyDescriptor>();
            foreach (PropertyDescriptor property in properties)
            {
                // filter properties here if needed

                wantedProperties.Add(property);
            }
            return new PropertyDescriptorCollection(wantedProperties.ToArray());
        }

        public object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
        }
        #endregion
    }
}
