using System;
using System.ComponentModel;

namespace AGS.Types
{
    public class FontSizeTypeConverter : TypeConverter
    {
        public FontSizeTypeConverter()
        {
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

        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            if (value is int && destinationType == typeof(string))
            {
                int fontSize = (int)value;
                // TODO: find a better solution for the font format check, perhaps store font type as Font's property,
                // or add a direct (unserialized) reference to related FontFile
                if (fontSize == 0 ||
                    (context.Instance is Font && !((Font)context.Instance).ProjectFilename.ToLower().EndsWith(".ttf")))
                {
                    return "N/A";
                }
                return fontSize.ToString();
            }
            if (value is string && destinationType == typeof(int))
            {
                int fontSize;
                if (int.TryParse((value as string), out fontSize))
                    return fontSize;
                return 0;
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            if (value is string)
            {
                int fontSize;
                if (int.TryParse((value as string), out fontSize))
                    return fontSize;
                return 0;
            }
            return base.ConvertFrom(context, culture, value);
        }
    }
}
