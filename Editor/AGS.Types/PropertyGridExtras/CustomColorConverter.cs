using System;
using System.ComponentModel;
using System.Drawing;

namespace AGS.Types
{
    public class CustomColorConverter : TypeConverter
    {
        public static GameColorDepth ColorMode = GameColorDepth.TrueColor;

        public CustomColorConverter()
        {
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
            if (value is Color && destinationType == typeof(string))
            {
                return ColorToString((Color)value);
            }
            if (value is int && destinationType == typeof(string))
            {
                return AgsColorNumberToString((int)value);
            }
            else if (value is string && destinationType == typeof(Color))
            {
                return ColorFromString(value as string).ToRGB();
            }
            else if (value is string && destinationType == typeof(int))
            {
                return AgsColorNumberFromString(value as string);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {
            if (value is string)
            {
                if (context.PropertyDescriptor.PropertyType == typeof(Color))
                {
                    return ColorFromString(value as string).ToRGB();
                }
                else if (context.PropertyDescriptor.PropertyType == typeof(int))
                {
                    return AgsColorNumberFromString(value as string);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        private Color ColorFromString(string value)
        {
            value = value.Trim();
            if (string.IsNullOrEmpty(value))
                return Color.Black; // return black color on failure

            // First check explicit notations
            if (value.StartsWith("#"))
            {
                return Utilities.ColorFromHTMLHex(value);
            }
            else if (value.StartsWith("0x"))
            {
                return Utilities.ColorFromARGBHex(value);
            }

            // Try parse one as a traditional AGS color number (decimal)
            int colorNumber;
            if (int.TryParse(value, out colorNumber))
                return AGSColor.ColorMapper.MapAgsColourNumberToRgbColor(colorNumber);

            // If it's not a single number of any kind, then try to parse as color components
            // separated either by comma or semicolon
            Color color = AGS.Types.Utilities.ColorFromSeparatedRGBA(value, ',');
            if (color.IsEmpty)
                color = AGS.Types.Utilities.ColorFromSeparatedRGBA(value, ';');
            if (!color.IsEmpty)
                return color;

            return Color.Black; // return black color on failure
        }

        private string ColorToString(Color color)
        {
            // In AGS v3.* there's no "alpha" support in color properties.
            return string.Format($"{color.R}; {color.G}; {color.B}");
        }

        private int AgsColorNumberFromString(string value)
        {
            if (ColorMode == GameColorDepth.Palette)
                return int.Parse(value);
            Color color = ColorFromString(value).ToRGB();
            return AGSColor.ColorMapper.MapRgbColorToAgsColourNumber(color);
        }

        private string AgsColorNumberToString(int value)
        {
            if (ColorMode == GameColorDepth.Palette)
                return value.ToString();
            return ColorToString(AGSColor.ColorMapper.MapAgsColourNumberToRgbColor(value));
        }
    }
}
