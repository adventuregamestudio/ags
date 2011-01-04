using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class CustomPropertySchemaItem : ICloneable
    {
        private string _name;
        private string _description;
        private string _defaultValue;
        private CustomPropertyType _type;
        private bool _appliesToCharacters = true;
        private bool _appliesToHotspots = true;
        private bool _appliesToObjects = true;
        private bool _appliesToInvItems = true;
        private bool _appliesToRooms = true;

        public CustomPropertySchemaItem()
        {
            _name = string.Empty;
            _description = string.Empty;
            _defaultValue = string.Empty;
        }

        public string Name
        {
            get { return _name; }
            set { _name = value; }
        }

        public string Description
        {
            get { return _description; }
            set { _description = value; }
        }

        public string DefaultValue
        {
            get { return _defaultValue; }
            set { _defaultValue = value; }
        }

        public bool AppliesToCharacters
        {
            get { return _appliesToCharacters; }
            set { _appliesToCharacters = value; }
        }

        public bool AppliesToHotspots
        {
            get { return _appliesToHotspots; }
            set { _appliesToHotspots = value; }
        }

        public bool AppliesToInvItems
        {
            get { return _appliesToInvItems; }
            set { _appliesToInvItems = value; }
        }

        public bool AppliesToObjects
        {
            get { return _appliesToObjects; }
            set { _appliesToObjects = value; }
        }

        public bool AppliesToRooms
        {
            get { return _appliesToRooms; }
            set { _appliesToRooms = value; }
        }

        public CustomPropertyType Type
        {
            get { return _type; }
            set { _type = value; }
        }

        [AGSNoSerialize]
        public string AppliesToAsString
        {
            get
            {
                string toReturn = string.Empty;
                toReturn += _appliesToCharacters ? "C" : "  ";
                toReturn += _appliesToHotspots ? "H" : "  ";
                toReturn += _appliesToInvItems ? "I" : "  ";
                toReturn += _appliesToObjects ? "O" : "  ";
                toReturn += _appliesToRooms ? "R" : "  ";
                return toReturn;
            }
        }

        public object GetTypedDefaultValue()
        {
            if (_type == CustomPropertyType.Boolean)
            {
                if (_defaultValue == "1")
                {
                    return true;
                }
                return false;
            }
            if (_type == CustomPropertyType.Number)
            {
                int result = 0;
                Int32.TryParse(_defaultValue, out result);
                return result;
            }
            return _defaultValue;
        }

        public CustomPropertySchemaItem(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

        public object Clone()
        {
            CustomPropertySchemaItem copy = new CustomPropertySchemaItem();
            copy.DefaultValue = this.DefaultValue;
            copy.Description = this.Description;
            copy.Name = this.Name;
            copy.Type = this.Type;
            copy.AppliesToCharacters = this.AppliesToCharacters;
            copy.AppliesToHotspots = this.AppliesToHotspots;
            copy.AppliesToInvItems = this.AppliesToInvItems;
            copy.AppliesToObjects = this.AppliesToObjects;
            copy.AppliesToRooms = this.AppliesToRooms;
            return copy;
        }
    }
}
