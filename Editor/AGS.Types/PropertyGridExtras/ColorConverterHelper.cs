using System;
using System.Drawing;

namespace AGS.Types
{
    internal static class ColorConverterHelper
    {
        public static GameColorDepth ColorMode = GameColorDepth.TrueColor;
        public const int SpecialColorSlotCount = 32;

        public static Color ColorFromString(string value)
        {
            value = value.Trim();
            if (string.IsNullOrEmpty(value))
                return Color.FromArgb(0); // return transparent color on failure
            // Special case: input "0" is a shortcut for "transparent"
            if (value == "0")
                return Color.FromArgb(0);

            // First check explicit notations
            if (value.StartsWith("#"))
            {
                return Utilities.ColorFromHTMLHex(value);
            }
            else if (value.StartsWith("0x"))
            {
                return Utilities.ColorFromARGBHex(value);
            }

            // If there's no explicit notation prefix, then try to parse as a HTML hex (expect failure!)
            Color color = Utilities.ColorFromHTMLHex(value);
            if (!color.IsEmpty)
                return color;

            // If it's not hex of any kind, then try to parse as color components
            // separated either by comma or semicolon
            color = AGS.Types.Utilities.ColorFromSeparatedRGBA(value, ',');
            if (color.IsEmpty)
                color = AGS.Types.Utilities.ColorFromSeparatedRGBA(value, ';');
            if (!color.IsEmpty)
                return color;
            return Color.FromArgb(0); // return transparent color on failure
        }

        public static string ColorToString(Color color)
        {
            // We make transparent color a special case, for user's convenience
            switch (color.A)
            {
                case 0: return "0";
                case 255: return string.Format($"{color.R}; {color.G}; {color.B}");
                default: return string.Format($"{color.R}; {color.G}; {color.B}; {color.A}");
            }
        }

        public static int AgsColorNumberFromString(string value)
        {
            if (ColorMode == GameColorDepth.Palette)
                return int.Parse(value);
            Color color = ColorFromString(value);
            return AGSColor.ColorMapper.MapRgbColorToAgsColourNumber(color);
        }

        public static int AgsColorNumberFromRGBString(string value)
        {
            Color color = ColorFromString(value);
            return AGSColor.ColorMapper.MapRgbColorToAgsColourNumber(color);
        }

        public static string AgsColorNumberToString(int value)
        {
            if (ColorMode == GameColorDepth.Palette)
                return value.ToString();
            return ColorToString(AGSColor.ColorMapper.MapAgsColourNumberToRgbColor(value));
        }

        public static string AgsColorNumberToRGBString(int value)
        {
            return ColorToString(AGSColor.ColorMapper.MapAgsColourNumberToRgbColor(value));
        }
    }
}
