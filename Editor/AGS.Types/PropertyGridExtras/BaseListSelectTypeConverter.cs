using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Design;
using System.Reflection;

namespace AGS.Types
{
    // TKey here is the choice ID, and TValue is displayable representation
    // (string, or an object convertable to string) for that choice
    public abstract class BaseListSelectTypeConverter<TKey, TValue> : TypeConverter
        where TValue : class
    {
        protected abstract Dictionary<TKey, TValue> GetValueList(ITypeDescriptorContext context);

        public BaseListSelectTypeConverter() : base()
        {
        }

        public override TypeConverter.StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
        {
            return new StandardValuesCollection(GetValueList(context).Keys);
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

        private TKey GetKeyForValue(ITypeDescriptorContext context, TValue displayObject)
        {
            foreach (KeyValuePair<TKey, TValue> entry in GetValueList(context))
            {
                if (entry.Value == displayObject)
                {
                    return entry.Key;
                }
            }
            throw new InvalidOperationException("Entry not found: " + displayObject);
        }

        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            if (value is TKey && destinationType == typeof(string))
            {
                // return name in code for this int
                var stringLookupDictionary = GetValueList(context);
                TKey key = (TKey)value;
                if (stringLookupDictionary.ContainsKey(key))
                {
                    return stringLookupDictionary[key];
                }
                return string.Format("{0} [unknown]", key);
            }
            if (value is TValue && destinationType == typeof(string))
            {
                // Convert from name in code to display name
                return value;
            }
            if (value is TValue && destinationType is TKey)
            {
                return GetKeyForValue(context, (TValue)value);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            if (value is TValue)
            {
                return GetKeyForValue(context, value as TValue);
            }
            return base.ConvertFrom(context, culture, value);
        }
    }
}