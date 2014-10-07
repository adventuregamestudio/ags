using System;
using System.ComponentModel;
using System.Drawing;

namespace AGS.Types
{
    public class CustomResolutionTypeConverter : TypeConverter
    {
        public CustomResolutionTypeConverter()
        {
        }
        
        // Although this type converter does not provide any standard values,
        // overriding this method helps to forbid entering values directly into
        // property field and forces user to use UI value editor instead
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

        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            if (value is Size && destinationType == typeof(string))
            {
                return Utilities.ResolutionToUserString((Size)value);
            }
            if (value is string && destinationType == typeof(Size))
            {
                return Utilities.UserStringToResolution((string)value);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            if (value is string)
            {
                return Utilities.UserStringToResolution((string)value);
            }
            return base.ConvertFrom(context, culture, value);
        }
    }
}