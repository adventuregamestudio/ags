using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using AGS.Types;

namespace AGS.Editor
{
    internal class CustomPropertyDescriptor : PropertyDescriptor
    {
        private CustomProperties _properties;
        private CustomPropertySchemaItem _schemaItem;

        public CustomPropertyDescriptor(CustomPropertySchemaItem schemaItem, CustomProperties properties)
            : base(schemaItem.Name, new Attribute[] { new DescriptionAttribute(schemaItem.Description), new DefaultValueAttribute(schemaItem.GetTypedDefaultValue()) })
        {
            _properties = properties;
            _schemaItem = schemaItem;
        }

        public override bool CanResetValue(object component)
        {
            return false;
        }

        public override Type ComponentType
        {
            get { return typeof(CustomPropertyBag); }
        }

        public override object GetValue(object component)
        {
            string currentValue;
            if (_properties.PropertyValues.ContainsKey(_schemaItem.Name))
            {
                currentValue = _properties.PropertyValues[_schemaItem.Name].Value;
            }
            else
            {
                currentValue = _schemaItem.DefaultValue;
            }

            if (_schemaItem.Type == CustomPropertyType.Boolean)
            {
                if (currentValue == "1")
                {
                    return true;
                }
                return false;
            }
            if (_schemaItem.Type == CustomPropertyType.Number)
            {
                return Convert.ToInt32(currentValue);
            }
            return currentValue;
        }

        public override bool IsReadOnly
        {
            get { return false; }
        }

        public override Type PropertyType
        {
            get
            {
                switch (_schemaItem.Type)
                {
                    case CustomPropertyType.Boolean:
                        return typeof(bool);
                    case CustomPropertyType.Number:
                        return typeof(int);
                    case CustomPropertyType.Text:
                        return typeof(string);
                    default:
                        throw new AGSEditorException("Unknown custom property type");
                }
            }
        }

        public override void ResetValue(object component)
        {
            throw new Exception("The method or operation is not implemented.");
        }

        public override void SetValue(object component, object value)
        {
            string stringValue;
            if (value.GetType() == typeof(bool))
            {
                stringValue = ((bool)value) ? "1" : "0";
            }
            else
            {
                stringValue = value.ToString();
            }
            if (stringValue == _schemaItem.DefaultValue)
            {
                if (_properties.PropertyValues.ContainsKey(_schemaItem.Name))
                {
                    _properties.PropertyValues.Remove(_schemaItem.Name);
                }
            }
            else
            {
                if (_properties.PropertyValues.ContainsKey(_schemaItem.Name))
                {
                    _properties.PropertyValues[_schemaItem.Name].Value = stringValue;
                }
                else
                {
                    CustomProperty newProperty = new CustomProperty();
                    newProperty.Name = _schemaItem.Name;
                    newProperty.Value = stringValue;
                    _properties.PropertyValues.Add(newProperty.Name, newProperty);
                }
            }
        }

        public override bool ShouldSerializeValue(object component)
        {
            return true;
        }
    }

}
