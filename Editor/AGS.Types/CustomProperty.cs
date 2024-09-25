using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [Serializable]
    public class CustomProperty : ICloneable
    {
        private string _name;
        private string _value;

        public CustomProperty()
        {
        }

        public CustomProperty(string name, string value)
        {
            _name = name;
            _value = value;
        }

        public string Name
        {
            get { return _name; }
            set { _name = value; }
        }

        public string Value
        {
            get { return _value; }
            set { _value = value; }
        }

        public CustomProperty(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public object Clone()
        {
            return new CustomProperty(_name, _value);
        }
    }
}
