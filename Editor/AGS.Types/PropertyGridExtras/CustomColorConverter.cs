using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;

namespace AGS.Types
{
    public class CustomColorConverter : TypeConverter
    {
        public CustomColorConverter()
        {
        }

        public static GameColorDepth ColorMode
        {
            get { return ColorConverterHelper.ColorMode; }
            set { ColorConverterHelper.ColorMode = value; }
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
            return base.CanConvertFrom(context, sourceType);
        }

        public override object ConvertTo(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value, Type destinationType)
        {
            if (value is Color && destinationType == typeof(string))
            {
                return ColorConverterHelper.ColorToString((Color)value);
            }
            else if (value is int && destinationType == typeof(string))
            {
                return ColorConverterHelper.AgsColorNumberToString((int)value);
            }
            else if (value is string && destinationType == typeof(Color))
            {
                return ColorConverterHelper.ColorFromString(value as string);
            }
            else if (value is string && destinationType == typeof(int))
            {
                return ColorConverterHelper.AgsColorNumberFromString(value as string);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            if (value is string)
            {
                if (context.PropertyDescriptor.PropertyType == typeof(Color))
                {
                    return ColorConverterHelper.ColorFromString(value as string);
                }
                else if (context.PropertyDescriptor.PropertyType == typeof(int))
                {
                    return ColorConverterHelper.AgsColorNumberFromString(value as string);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes)
        {
            if ((ColorMode != GameColorDepth.Palette) && (context != null))
            {
                var descriptors = new List<PropertyDescriptor>();
                descriptors.Add(new ColorNumberDescriptor(context, new Attribute[] { new ReadOnlyAttribute(true) }));
                descriptors.Add(new ColorRGBDescriptor(context, new Attribute[] {
                    new TypeConverterAttribute(typeof(ColorRGBConverter)), new ReadOnlyAttribute(true)
                }));
                return new PropertyDescriptorCollection(descriptors.ToArray());
            }
            return base.GetProperties(context, value, attributes);
        }

        public override bool GetPropertiesSupported(ITypeDescriptorContext context)
        {
            // Do not expand to sub-properties in Palette mode, because it's always color index then.
            return ColorMode != GameColorDepth.Palette;
        }
    }
}
