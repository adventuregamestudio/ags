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
                return ColorFromString(value as string);
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
                    return ColorFromString(value as string);
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
            // First try reading a hexadecimal number
            try
            {
                int argb = Convert.ToInt32(value, 16);
                return Color.FromArgb(argb);
            }
            catch (Exception)
            {
            }

            // If failed, then try to parse as color components
            var rgb = value.Split(';');
            switch (rgb.Length)
            {
                case 3: return Color.FromArgb(int.Parse(rgb[0]), int.Parse(rgb[1]), int.Parse(rgb[2]));
                case 4: return Color.FromArgb(int.Parse(rgb[3]), int.Parse(rgb[0]), int.Parse(rgb[1]), int.Parse(rgb[2]));
                default: return Color.FromArgb(0); // return transparent color on failure
            }
        }

        private string ColorToString(Color color)
        {
            // We make transparent color a special case, for user's convenience
            switch (color.A)
            {
                case 0: return "0";
                case 255: return string.Format($"{color.R}; {color.G}; {color.B}");
                default: return string.Format($"{color.R}; {color.G}; {color.B}; {color.A}");
            }
        }

        private int AgsColorNumberFromString(string value)
        {
            if (ColorMode == GameColorDepth.Palette)
                return int.Parse(value);
            Color color = ColorFromString(value);
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
