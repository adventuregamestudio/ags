using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;
using System.Reflection;

namespace AGS.Types
{
    public abstract class BaseListSelectTypeConverter : TypeConverter
    {
        protected abstract Dictionary<int, string> GetValueList();

        public BaseListSelectTypeConverter() : base()
        {
        }

        public override TypeConverter.StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
        {
            return new StandardValuesCollection(GetValueList().Keys);
        }

        public override bool GetStandardValuesSupported(ITypeDescriptorContext context)
        {
            return true;
        }

        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context)
        {
            return true;
        }

        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
            {
                return true;
            }
            return false;
        }

        public override bool IsValid(ITypeDescriptorContext context, object value)
        {
            return base.IsValid(context, value);
        }

        private int GetValueForString(string displayName)
        {
            foreach (KeyValuePair<int, string> entry in GetValueList())
            {
                if (entry.Value == displayName)
                {
                    return entry.Key;
                }
            }
            throw new InvalidOperationException("Entry not found: " + displayName);
        }

        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            if (value is int && destinationType == typeof(string))
            {
                // return name in code for this int
                var stringLookupDictionary = GetValueList();
                int intVal = (int)value;
                if (stringLookupDictionary.ContainsKey(intVal))
                {
                    return stringLookupDictionary[intVal];
                }
                return string.Format("{0} [unknown]", intVal);
            }
            if (value is string && destinationType == typeof(string))
            {
                // Convert from name in code to display name
                return value;
            }
            if (value is string && destinationType == typeof(int))
            {
                return GetValueForString((string)value);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            if (value is string)
            {
                return GetValueForString((string)value);
            }
            return base.ConvertFrom(context, culture, value);
        }
    }
}